/**********************************************************************
 Freeciv - Copyright (C) 2005  - M.C. Kaufman
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_MYSQL
#include <mysql/mysql.h>
#endif

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include "fcintl.h"
#include "hash.h"
#include "log.h"
#include "md5.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "support.h"
#include "timing.h"

#include "capstr.h"
#include "city.h"
#include "connection.h"
#include "government.h"
#include "packets.h"

#include "connecthand.h"
#include "meta.h"
#include "plrhand.h"
#include "score.h"
#include "sernet.h"
#include "settings.h"
#include "srv_main.h"

#include "database.h"

struct fcdb_params fcdb = {

  /* Where our mysql database is located and how to get to it. */
  .host = "localhost",
  .user = "freeciv",
  .password = "password",

  /* The database where our tables are located. */
  .dbname = "freeciv"
};

#define AUTH_TABLE      "auth"
#define LOGIN_TABLE     "loginlog"

#define GUEST_NAME "guest"

#define MIN_PASSWORD_LEN  6  /* minimum length of password */
#define MIN_PASSWORD_CAPS 0  /* minimum number of capital letters required */
#define MIN_PASSWORD_NUMS 0  /* minimum number of numbers required */

#define MAX_AUTH_TRIES 3
#define MAX_WAIT_TIME 30   /* max time we'll wait on a password */

/* after each wrong guess for a password, the server waits this
 * many seconds to reply to the client */
static const int auth_fail_wait[] = { 1, 1, 2, 3 };

/**************************************************
 The auth db statuses are:

 1: an error occurred, possibly we couldn't access the database file.
 2: we were able to successfully insert an entry. or we found the entry 
    we were searching for
 3: the user we were searching for was not found.
***************************************************/
enum authdb_status {
  AUTH_DB_ERROR,
  AUTH_DB_SUCCESS,
  AUTH_DB_NOT_FOUND
};

static bool is_good_password(const char *password, char *msg);

static bool check_password(struct connection *pconn,
                           const char *password, int len);
static enum authdb_status load_user(struct connection *pconn);
static bool save_user(struct connection *pconn);


#ifdef HAVE_MYSQL

#define FCDB_ETM_FILE_IDENTIFIER  84
#define FCDB_ETM_VERSION          1
#define FCDB_ETM_HEADER_LEN       4
#define FCDB_ETM_DICT_LEN         32
#define FCDB_ETM_RDICT_LEN        256

#define FCDB_ETM_SPECIAL1_FLAG    (1 << 5)
#define FCDB_ETM_SPECIAL2_FLAG    (1 << 6)
#define FCDB_ETM_RIVER_FLAG       (1 << 7)

#define FCDB_ETM_UNIT_FLAG             (1 << 5)
#define FCDB_ETM_CITY_FLAG             (1 << 6)
#define FCDB_ETM_CHANGED_TERRAIN_FLAG  (1 << 7)

/* For timing of db accesses. */
static struct timer *fcdb_timer = NULL;

static void fcdb_error(char *fmt, ...);
static bool fcdb_connect(MYSQL *mysql);
static void fcdb_close(MYSQL *mysql);
static const char *fcdb_escape(MYSQL *sock, const char *str);
static bool fcdb_execute(MYSQL *sock, const char *stmt);
static bool fcdb_commit(MYSQL *sock);
static bool fcdb_insert_player(MYSQL *sock, struct player *pplayer);

static char *etm_encode_terrain_map(void);
static char *etm_encode_turn_map(const char *termap,
                                 struct tile_list *changed_terrain_list);
static unsigned long get_termap_len(void);
static unsigned long get_turnmap_len(void);
static unsigned long get_tile_termap_index(const struct tile *ptile);
static char etm_encode_terrain(const struct tile *ptile,
                               const char *rdict);

#define fcdb_reset_escape_buffer() (void)fcdb_escape(NULL, NULL)

#define fcdb_execute_or_return(sock, stmt, val) do {\
  if (!fcdb_execute((sock), (stmt))) {\
    return (val);\
  }\
} while(0)

#define fcdb_connect_or_return(sock, val) do {\
  if (!fcdb_connect(sock)) {\
    return val;\
  }\
} while(0)

#endif /* HAVE_MYSQL */


/**************************************************************************
  handle authentication of a user; called by handle_login_request()
  if authentication is enabled.

  if the connection is rejected right away, return FALSE, otherwise return TRUE
**************************************************************************/
bool authenticate_user(struct connection *pconn, char *username)
{
  char tmpname[MAX_LEN_NAME] = "\0";

  /* assign the client a unique guest name/reject if guests aren't allowed */
  if (is_guest_name(username)) {
    if (srvarg.auth.allow_guests) {

      sz_strlcpy(tmpname, username);
      get_unique_guest_name(username);

      if (strncmp(tmpname, username, MAX_LEN_NAME) != 0) {
        notify_conn(pconn->self, 
                    _("Warning: the guest name '%s' has been "
                      "taken, renaming to user '%s'."), tmpname, username);
      }
      sz_strlcpy(pconn->username, username);
      establish_new_connection(pconn);
    } else {
      reject_new_connection(_("Guests are not allowed on this server. "
                              "Sorry."), pconn);
      freelog(LOG_NORMAL, _("%s was rejected: Guests not allowed."), username);
      return FALSE;
    }
  } else {
    /* we are not a guest, we need an extra check as to whether a 
     * connection can be established: the client must authenticate itself */
    char buffer[MAX_LEN_MSG];

    sz_strlcpy(pconn->username, username);

    switch (load_user(pconn)) {
    case AUTH_DB_ERROR:
      if (srvarg.auth.allow_guests) {
        sz_strlcpy(tmpname, pconn->username);
        get_unique_guest_name(tmpname); /* don't pass pconn->username here */
        sz_strlcpy(pconn->username, tmpname);

        freelog(LOG_ERROR, "Error reading database; connection -> guest");
        notify_conn(pconn->self, 
                    _("There was an error reading the user "
                      "database, logging in as guest connection '%s'."), 
                    pconn->username);
        establish_new_connection(pconn);
      } else {
        reject_new_connection(_("There was an error reading the user database "
                                "and guest logins are not allowed. Sorry"), 
                              pconn);
        freelog(LOG_NORMAL, 
                _("%s was rejected: Database error and guests not allowed."),
                pconn->username);
        return FALSE;
      }
      break;
    case AUTH_DB_SUCCESS:
      freelog(LOG_VERBOSE, "AUTH_DB_SUCCESS: "
	      "About to send authentification packet to %s", pconn->username);
      /* we found a user */
      my_snprintf(buffer, sizeof(buffer), _("Enter password for %s:"),
                  pconn->username);
      dsend_packet_authentication_req(pconn, AUTH_LOGIN_FIRST, buffer);
      pconn->server.auth_settime = time(NULL);
      pconn->server.status = AS_REQUESTING_OLD_PASS;
      freelog(LOG_VERBOSE, "Auth packet %s sent",pconn->username);
      break;
    case AUTH_DB_NOT_FOUND:
      /* we couldn't find the user, he is new */
      if (srvarg.auth.allow_newusers) {
        freelog(LOG_VERBOSE, "AUTH_DB_NOT_FOUND: "
		"About to send authentification packet to %s", pconn->username);
        sz_strlcpy(buffer, _("Enter a new password (and remember it)."));
        dsend_packet_authentication_req(pconn, AUTH_NEWUSER_FIRST, buffer);
        pconn->server.auth_settime = time(NULL);
        pconn->server.status = AS_REQUESTING_NEW_PASS;
        freelog(LOG_VERBOSE, "Auth packet %s sent", pconn->username);
      } else {
        reject_new_connection(_("This server allows only preregistered "
                                "users. Sorry."), pconn);
        freelog(LOG_NORMAL,
                _("%s was rejected: Only preregistered users allowed."),
                pconn->username);

        return FALSE;
      }
      break;
    default:
      assert(0);
      break;
    }
    return TRUE;
  }

  return TRUE;
}

/**************************************************************************
  Create the random salt for a new password.
**************************************************************************/
static void create_salt(struct connection *pconn)
{
  RANDOM_STATE old_state;

  if (!pconn) {
    return;
  }

  if (!srvarg.auth.salted) {
    pconn->server.salt = 0;
    return;
  }

  /* Save and restore the global rand state. */
  old_state = get_myrand_state();

  /* FIXME: Is this cryptographically sound? */
  mysrand(time(NULL) + pconn->id);
  pconn->server.salt = (int) myrand(MAX_UINT32);

  set_myrand_state(old_state);
}

#ifdef HAVE_MYSQL
/**************************************************************************
  Create an md5 check sum from the password and salt and put it in 'dest'.
  NB: 'dest' is assumed to be able to hold DIGEST_HEX_BYTES + 1 bytes
  (don't forget the +1 for the terminating '\0').
**************************************************************************/
static void create_salted_md5sum(const char *password, int passlen,
                                 int salt, char *dest)
{
  const int SALT_LEN = sizeof(int);
  char buf[MAX_LEN_PASSWORD + SALT_LEN + 64];
  int rem;

  memcpy(buf, &salt, SALT_LEN);
  rem = MIN(passlen, sizeof(buf) - SALT_LEN);
  memcpy(buf + SALT_LEN, password, rem);
  create_md5sum(buf, SALT_LEN + rem, dest);
}
#endif /* HAVE_MYSQL */

/**************************************************************************
  Receives a password from a client and verifies it.
**************************************************************************/
bool handle_authentication_reply(struct connection *pconn, char *password)
{
  char msg[MAX_LEN_MSG];

  if (pconn->server.status == AS_REQUESTING_NEW_PASS) {

    /* check if the new password is acceptable */
    if (!is_good_password(password, msg)) {
      if (pconn->server.auth_tries++ >= MAX_AUTH_TRIES) {
        reject_new_connection(_("Sorry, too many wrong tries..."), pconn);
        freelog(LOG_NORMAL, _("%s was rejected: Too many wrong password "
                "verifies for new user."), pconn->username);
        return FALSE;
      } else {
        dsend_packet_authentication_req(pconn, AUTH_NEWUSER_RETRY, msg);
        return TRUE;
      }
    }

    /* the new password is good, create a database entry for
     * this user; we establish the connection in handle_db_lookup */
    sz_strlcpy(pconn->server.password, password);
    create_salt(pconn);

    if (!save_user(pconn)) {
      notify_conn(pconn->self, 
          _("Warning: There was an error in saving to the database. "
            "Continuing, but your stats will not be saved."));
      freelog(LOG_ERROR, "Error writing to database for: %s",
              pconn->username);
    }

    establish_new_connection(pconn);
  } else if (pconn->server.status == AS_REQUESTING_OLD_PASS) { 
    if (check_password(pconn, password, strlen(password))) {
      establish_new_connection(pconn);
    } else {
      pconn->server.status = AS_FAILED;
      pconn->server.auth_tries++;
      pconn->server.auth_settime = time(NULL)
          + auth_fail_wait[pconn->server.auth_tries];
    }
  } else {
    freelog(LOG_VERBOSE, "%s is sending unrequested auth packets", 
            pconn->username);
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
 checks on where in the authentication process we are.
**************************************************************************/
void process_authentication_status(struct connection *pconn)
{
  switch(pconn->server.status) {
  case AS_NOT_ESTABLISHED:
    /* nothing, we're not ready to do anything here yet. */
    break;
  case AS_FAILED:
    /* the connection gave the wrong password, we kick 'em off or
     * we're throttling the connection to avoid password guessing */
    if (pconn->server.auth_settime > 0
        && time(NULL) >= pconn->server.auth_settime) {

      if (pconn->server.auth_tries >= MAX_AUTH_TRIES) {
        pconn->server.status = AS_NOT_ESTABLISHED;
        reject_new_connection(_("Sorry, too many wrong tries..."), pconn);
        freelog(LOG_NORMAL,
                _("%s was rejected: Too many wrong password tries."),
                pconn->username);
        server_break_connection(pconn, ES_AUTH_FAILED);
      } else {
        struct packet_authentication_req request;

        pconn->server.status = AS_REQUESTING_OLD_PASS;
        request.type = AUTH_LOGIN_RETRY;
        sz_strlcpy(request.message,
                   _("Your password is incorrect. Try again."));
        send_packet_authentication_req(pconn, &request);
      }
    }
    break;
  case AS_REQUESTING_OLD_PASS:
  case AS_REQUESTING_NEW_PASS:
    /* Waiting on the client to send us a password.
       Don't wait too long. */
    if (time(NULL) >= pconn->server.auth_settime + MAX_WAIT_TIME) {
      pconn->server.status = AS_NOT_ESTABLISHED;
      reject_new_connection(_("Sorry, your connection timed out..."),
                            pconn);
      freelog(LOG_NORMAL,
          _("%s was rejected: Connection timeout waiting for password."),
          pconn->username);

      server_break_connection(pconn, ES_AUTH_FAILED);
    }
    break;
  case AS_ESTABLISHED:
    /* this better fail bigtime */
    assert(pconn->server.status != AS_ESTABLISHED);
    break;
  }
}

/**************************************************************************
  see if the name qualifies as a guest login name
**************************************************************************/
bool is_guest_name(const char *name)
{
  return (mystrncasecmp(name, GUEST_NAME, strlen(GUEST_NAME)) == 0);
}

/**************************************************************************
  return a unique guest name
  WARNING: do not pass pconn->username to this function: it won't return!
**************************************************************************/
void get_unique_guest_name(char *name)
{
  unsigned int i;

  /* first see if the given name is suitable */
  if (is_guest_name(name) && !find_conn_by_user(name)) {
    return;
  } 

  /* next try bare guest name */
  mystrlcpy(name, GUEST_NAME, MAX_LEN_NAME);
  if (!find_conn_by_user(name)) {
    return;
  }

  /* bare name is taken, append numbers */
  for (i = 1; ; i++) {
    my_snprintf(name, MAX_LEN_NAME, "%s%u", GUEST_NAME, i);


    /* attempt to find this name; if we can't we're good to go */
    if (!find_conn_by_user(name)) {
      break;
    }
  }
}

/**************************************************************************
 Verifies that a password is valid. Does some [very] rudimentary safety 
 checks. TODO: do we want to frown on non-printing characters?
 Fill the msg (length MAX_LEN_MSG) with any worthwhile information that 
 the client ought to know. 
**************************************************************************/
static bool is_good_password(const char *password, char *msg)
{
  int i, num_caps = 0, num_nums = 0;
   
  /* check password length */
  if (strlen(password) < MIN_PASSWORD_LEN) {
    my_snprintf(msg, MAX_LEN_MSG, _("Your password is too short, the "
        "minimum length is %d. Try again."), MIN_PASSWORD_LEN);
    return FALSE;
  }
 
  my_snprintf(msg, MAX_LEN_MSG, _("The password must have at least %d "
      "capital letters, %d numbers, and be at minimum %d [printable] "
      "characters long. Try again."),
      MIN_PASSWORD_CAPS, MIN_PASSWORD_NUMS, MIN_PASSWORD_LEN);

  for (i = 0; i < strlen(password); i++) {
    if (my_isupper(password[i])) {
      num_caps++;
    }
    if (my_isdigit(password[i])) {
      num_nums++;
    }
  }

  /* check number of capital letters */
  if (num_caps < MIN_PASSWORD_CAPS) {
    return FALSE;
  }

  /* check number of numbers */
  if (num_nums < MIN_PASSWORD_NUMS) {
    return FALSE;
  }

  if (!is_ascii_name(password)) {
    return FALSE;
  }

  return TRUE;
}

#ifdef HAVE_MYSQL
/**************************************************************************
  Returns TRUE if the connection's password salt is not empty.
***************************************************************************/
static bool has_salt(const struct connection *pconn)
{
  if (!pconn) {
    return FALSE;
  }
  return pconn->server.salt != 0;
}
#endif /* HAVE_MYQL */

/**************************************************************************
  Check if the password with length len matches that given in 
  pconn->server.password.
***************************************************************************/
static bool check_password(struct connection *pconn, 
                           const char *password, int passlen)
{
#ifdef HAVE_MYSQL
  bool password_ok = FALSE;
  char buffer[1024] = "";
  char checksum[DIGEST_HEX_BYTES + 1];
  MYSQL *sock, mysql;
  char escaped_name[MAX_LEN_NAME * 2 + 1];

  /* do the password checking right here */
  if (srvarg.auth.salted && has_salt(pconn)) {
    create_salted_md5sum(password, passlen, pconn->server.salt, checksum);
  } else {
    create_md5sum(password, passlen, checksum);
  }
  password_ok = (0 == strncmp(checksum, pconn->server.password,
                              DIGEST_HEX_BYTES));

  /* we don't really need the stuff below here to 
   * verify password, this is just logging */
  mysql_init(&mysql);

  /* attempt to connect to the server */
  if (!(sock = mysql_real_connect(&mysql, fcdb.host, fcdb.user,
                                  fcdb.password, fcdb.dbname, 0,
                                  NULL, 0))) {
    freelog(LOG_ERROR, "Can't connect to server! (%s)",
            mysql_error(&mysql));
    return password_ok;
  }

  mysql_real_escape_string(sock, escaped_name, pconn->username,
                           strlen(pconn->username));

  /* insert an entry into our log */
  my_snprintf(buffer, sizeof(buffer),
      "insert into %s (name, logintime, address, succeed) "
      "values ('%s',unix_timestamp(),'%s','%s')",
      LOGIN_TABLE, escaped_name, pconn->server.ipaddr,
      password_ok ? "S" : "F");
  if (mysql_query(sock, buffer)) {
    freelog(LOG_ERROR, "check_password insert loginlog failed for "
            "user: %s (%s)", pconn->username, mysql_error(sock));
    mysql_close(sock);
    return password_ok;
  }

  if (password_ok) {
    my_snprintf(buffer, sizeof(buffer),
        "update %s set accesstime=unix_timestamp(), address='%s', "
        "logincount=logincount+1 where name = '%s'",
        AUTH_TABLE, pconn->server.ipaddr, escaped_name);
    if (mysql_query(sock, buffer)) {
      freelog(LOG_ERROR, "check_password auth update failed for "
              "user: %s (%s)", pconn->username, mysql_error(sock));
      mysql_close(sock);
      return password_ok;
    }

    /* Create a new salt if this user doesn't have one. */
    if (srvarg.auth.salted && !has_salt(pconn)) {
      create_salt(pconn);
      create_salted_md5sum(password, passlen, pconn->server.salt,
                           checksum);
      my_snprintf(buffer, sizeof(buffer),
          "UPDATE %s SET password = '%s', salt = %d WHERE name = '%s'",
          AUTH_TABLE, checksum, pconn->server.salt, escaped_name);
      if (mysql_query(sock, buffer)) {
        freelog(LOG_ERROR, "password salt update failed for "
                "user: %s (%s)", pconn->username, mysql_error(sock));
        mysql_close(sock);
        return password_ok;
      }
    }
  }


  mysql_close(sock);
  return password_ok;
#else
  return TRUE;
#endif /* HAVE_MYSQL */
}

/**************************************************************************
 Loads a user from the database.
**************************************************************************/
static enum authdb_status load_user(struct connection *pconn)
{
#ifdef HAVE_MYSQL
  char buffer[512] = "";
  int num_rows = 0;
  MYSQL *sock, mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char escaped_name[MAX_LEN_NAME * 2 + 1];
  
  mysql_init(&mysql);

  /* attempt to connect to the server */
  if (!(sock = mysql_real_connect(&mysql, fcdb.host, fcdb.user,
                                  fcdb.password, fcdb.dbname,
                                  0, NULL, 0))) {
    freelog(LOG_ERROR, "Can't connect to server! (%s)",
            mysql_error(&mysql));
    return AUTH_DB_ERROR;
  } 
  
  mysql_real_escape_string(sock, escaped_name, pconn->username,
                           strlen(pconn->username));

  if (srvarg.auth.salted) {
    /* select password and salt from the entry */
    my_snprintf(buffer, sizeof(buffer),
                "select password, salt from %s where name = '%s'",
                AUTH_TABLE, escaped_name);
  } else {
    /* select just the password from the entry */
    my_snprintf(buffer, sizeof(buffer),
                "select password from %s where name = '%s'",
                AUTH_TABLE, escaped_name);
  }

  if (mysql_query(sock, buffer)) {
    freelog(LOG_ERROR, "db_load query failed for user: %s (%s)",
            pconn->username, mysql_error(sock));
    return AUTH_DB_ERROR;
  } 
  
  res = mysql_store_result(sock);
  num_rows = mysql_num_rows(res);
  
  /* if num_rows = 0, then we could find no such user */
  if (num_rows < 1) {
    mysql_free_result(res);
    mysql_close(sock);
    
    return AUTH_DB_NOT_FOUND;
  }
  
  /* if there are more than one row that matches this name,
   * it's an error continue anyway though */
  if (num_rows > 1) {
    freelog(LOG_ERROR, "db_load query found multiple entries "
            "(%d) for user: %s", num_rows, pconn->username);
  }

  /* if there are rows, then fetch them and use the first one */
  row = mysql_fetch_row(res);
  mystrlcpy(pconn->server.password, row[0],
            sizeof(pconn->server.password));
  if (srvarg.auth.salted) {
    if (row[1] != NULL) {
      pconn->server.salt = atoi(row[1]);
    } else {
      pconn->server.salt = 0;
    }
  }

  mysql_free_result(res);

  if (mysql_query(sock, buffer)) {
    freelog(LOG_ERROR, "db_load update accesstime failed for "
            "user: %s (%s)", pconn->username, mysql_error(sock));
  }

  mysql_close(sock);
#endif /* HAVE_MYSQL */
  return AUTH_DB_SUCCESS;
}

/**************************************************************************
 Saves pconn fields to the database. If the username already exists, 
 replace the data.
**************************************************************************/
static bool save_user(struct connection *pconn)
{
#ifdef HAVE_MYSQL
  char buffer[1024] = "";
  MYSQL *sock, mysql;
  char escaped_name[MAX_LEN_NAME * 2 + 1];
  char checksum[DIGEST_HEX_BYTES + 1];
  int passlen;

  mysql_init(&mysql);

  /* attempt to connect to the server */
  if (!(sock = mysql_real_connect(&mysql, fcdb.host, fcdb.user,
                                  fcdb.password, fcdb.dbname,
                                  0, NULL, 0))) {
    freelog(LOG_ERROR, "Can't connect to server! (%s)",
            mysql_error(&mysql));
    return FALSE;
  }

  mysql_real_escape_string(sock, escaped_name, pconn->username,
                           strlen(pconn->username));

  passlen = strlen(pconn->server.password);
  if (srvarg.auth.salted) {
    create_salted_md5sum(pconn->server.password, passlen,
                         pconn->server.salt, checksum);
  } else {
    create_md5sum(pconn->server.password, passlen, checksum);
  }

  /* insert new user into table. we insert the following things: name
   * md5sum of the password (with salt if enabled), the creation time
   * in seconds, the accesstime also in seconds from 1970, the users
   * address (twice) and the logincount */
  if (srvarg.auth.salted) {
    my_snprintf(buffer, sizeof(buffer), "insert into %s values "
                "(NULL, '%s', '%s', %d, NULL, unix_timestamp(), "
                "unix_timestamp(), '%s', '%s', 0)",
                AUTH_TABLE, escaped_name, checksum, pconn->server.salt,
                pconn->server.ipaddr, pconn->server.ipaddr);
  } else {
    my_snprintf(buffer, sizeof(buffer), "insert into %s values "
                "(NULL, '%s', '%s', NULL, unix_timestamp(), "
                "unix_timestamp(), '%s', '%s', 0)",
                AUTH_TABLE, escaped_name, checksum,
                pconn->server.ipaddr, pconn->server.ipaddr);
  }

  if (mysql_query(sock, buffer)) {
    freelog(LOG_ERROR, "db_save insert failed for new user: %s (%s)",
                       pconn->username, mysql_error(sock));
    mysql_close(sock);
    return FALSE;
  }

  /* insert an entry into our log */
  memset(buffer, 0, sizeof(buffer));
  my_snprintf(buffer, sizeof(buffer),
              "insert into %s (name, logintime, address, succeed) "
              "values ('%s',unix_timestamp(),'%s', 'S')", LOGIN_TABLE,
              escaped_name, pconn->server.ipaddr);

  if (mysql_query(sock, buffer)) {
    freelog(LOG_ERROR, "db_load insert loginlog failed for user: %s (%s)",
            pconn->username, mysql_error(sock));
  }

  mysql_close(sock);
#endif
  return TRUE;
}

#ifdef HAVE_MYSQL
/**************************************************************************
  ...
**************************************************************************/
static void fcdb_error(char *fmt, ...)
{
  va_list ap;
  char buf[1024];

  va_start(ap, fmt);
  my_vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  freelog(LOG_ERROR, "FCDB: %s", buf);

  srvarg.fcdb.enabled = FALSE;
  freelog(LOG_ERROR, "FCDB: Disabling database updates because of errors.");

  fcdb_reset_escape_buffer(); /* Don't leave it for someone else! */
}

/**************************************************************************
  NB turns off autocommit.
**************************************************************************/
static bool fcdb_connect(MYSQL *mysql)
{
  if (!srvarg.auth.enabled) {
    fcdb_error("Game database updating aborted because "
               "user authentication is disabled.");
    return FALSE;
  }

  if (!fcdb_timer) {
    fcdb_timer = new_timer(TIMER_USER, TIMER_ACTIVE);
  }

  clear_timer_start(fcdb_timer);

  mysql_init(mysql);

  /* Attempt to connect to the server. */
  if (!mysql_real_connect(mysql, fcdb.host, fcdb.user,
                          fcdb.password, fcdb.dbname,
                          0, NULL, 0)) {
    fcdb_error("Cannot connect to database server: %s",
               mysql_error(mysql));
    return FALSE;
  }

  /* To prevent errors midway through an update causing the
     database data to become inconsistent, turn off autocommit. */
  if (mysql_autocommit(mysql, FALSE)) {
    fcdb_error("Failed to turn off database autocommit: %s",
               mysql_error(mysql));
    mysql_close(mysql);
    return FALSE;
  }
  
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static void fcdb_close(MYSQL *mysql)
{
  mysql_close(mysql);

  stop_timer(fcdb_timer);
  freelog(LOG_VERBOSE, "FCDB: Database access took %f seconds.",
          read_timer_seconds(fcdb_timer));
}

/**************************************************************************
  NB NOT THREAD SAFE. But may be called more than once in the same
  expression (up to 2048 *worst case* output bytes ==> worst case is
  overflow after 1023 bytes input). Call fcdb_reset_escape_buffer() to
  reset the buffer when done with the returned pointer(s). Also assumes
  str is null-terminated.
**************************************************************************/
static const char *fcdb_escape(MYSQL *sock, const char *str)
{
  static char tmp[2048], *p = tmp;
  unsigned long len;
  char *ret = NULL;

  if (!sock || !str) {
    p = tmp;
    return NULL;
  }
  
  len = strlen(str);
  if (2 * len + 1 >= sizeof(tmp) - (unsigned)(p - tmp)) {
    fcdb_error("Buffer overflow in call to fcdb_escape! "
               "Someone didn't take out the trash, "
               "or put something too big into it. :(");
    ret = NULL;
  } else {
    ret = p;
    p += 1 + mysql_real_escape_string(sock, p, str, len);
  }
  return ret;
}

/**************************************************************************
  NB calls fcdb_close(sock) on error.
**************************************************************************/
static bool fcdb_execute(MYSQL *sock, const char *stmt)
{
  freelog(LOG_DEBUG, "FCDB: Executing statement: \"%s\"", stmt);
  if (mysql_query(sock, stmt)) {
    fcdb_error("Database execute error: %s.\nStatement was: %s",
               mysql_error(sock), stmt);
    fcdb_close(sock);
    return FALSE;
  }
  return TRUE;
}

/**************************************************************************
  NB does NOT call fcdb_close on error.
**************************************************************************/
static bool fcdb_commit(MYSQL *sock)
{
  if (mysql_commit(sock)) {
    fcdb_error("Database commit failed: %s.", mysql_error(sock));
    return FALSE;
  }
  return TRUE;
}

/**************************************************************************
  NB Calls fcdb_close on error.
**************************************************************************/
static bool fcdb_insert_player(MYSQL *sock, struct player *pplayer)
{
  char buf[1024];

  if (pplayer->fcdb.player_id > 0) {
    fcdb_error("Tried to insert player %s into the "
               "database twice: already has id %d.",
               pplayer->name, pplayer->fcdb.player_id);
    fcdb_close(sock);
    return FALSE;
  }

  my_snprintf(buf, sizeof(buf), "INSERT INTO players"
      "(game_id, name, nation) VALUES (%d, '%s', '%s')",
      game.server.fcdb.id, fcdb_escape(sock, pplayer->name),
      fcdb_escape(sock, get_nation_name(pplayer->nation)));

  fcdb_execute_or_return(sock, buf, FALSE);

  pplayer->fcdb.player_id = mysql_insert_id(sock);

  if (pplayer->is_connected
      && 0 != strcmp(pplayer->username, ANON_USER_NAME)) {
    my_snprintf(buf, sizeof(buf), "UPDATE players "
        "SET creating_user_name = '%s' WHERE id = %d",
        fcdb_escape(sock, pplayer->username),
        pplayer->fcdb.player_id);
    fcdb_execute_or_return(sock, buf, FALSE);
  }
  
  if (pplayer->team != TEAM_NONE) {
    my_snprintf(buf, sizeof(buf), "UPDATE players "
        "SET team_id = ("
        "  SELECT id FROM teams "
        "    WHERE teams.game_id = %d AND teams.name = '%s'"
        ") WHERE id = %d",
        game.server.fcdb.id,
        fcdb_escape(sock, get_team_name(pplayer->team)),
        pplayer->fcdb.player_id);
    fcdb_execute_or_return(sock, buf, FALSE);
  }

  fcdb_reset_escape_buffer();
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool fcdb_insert_terrain_map(MYSQL *sock)
{
  MYSQL_BIND bind[2];
  MYSQL_STMT *stmt = NULL;
  char *tmap = NULL, *tmap_comp = NULL;
  unsigned long len, len_comp, len_comp_max;

  static const char *INSERT_STMT
    = "INSERT INTO terrain_maps (game_id, map) VALUES (?, ?)";

  if (!srvarg.fcdb.save_maps) {
    return TRUE;
  }

  /* Create the terrain map that we save in the database. */
  tmap = etm_encode_terrain_map();
  if (!tmap) {
    fcdb_error("Failed to encode terrain map.");
    goto ERROR;
  }

  len = get_termap_len();
  len_comp_max = 1001 * len / 1000 + 64;
  tmap_comp = fc_malloc(len_comp_max);

  len_comp = len_comp_max; /* Satisfy zlib. */

  switch (compress((unsigned char *)tmap_comp, &len_comp,
                   (unsigned char *)tmap, len)) {
  case Z_OK:
    break;
  case Z_MEM_ERROR:
    fcdb_error("Failed to compress encoded terrain map: "
               "not enough memory.");
    goto ERROR;
    break;
  case Z_BUF_ERROR:
    fcdb_error("Failed to compress encoded terrain map: "
               "output buffer size exceeded.");
    goto ERROR;
    break;
  default:
    assert(0);
    break;
  }

  freelog(LOG_VERBOSE, "FCDB: Encoded terrain map compressed to "
          "%lu bytes down from %lu bytes.",
          len_comp, len);

  stmt = mysql_stmt_init(sock);
  if (!stmt) {
    fcdb_error("mysql_stmt_init() failed: out of memory.");
    goto ERROR;
  }

  if (mysql_stmt_prepare(stmt, INSERT_STMT, strlen(INSERT_STMT))) {
    fcdb_error("mysql_stmt_prepare() failed: %s.",
               mysql_stmt_error(stmt));
    goto ERROR;
  }

  memset(bind, 0, sizeof(bind));

  bind[0].buffer_type = MYSQL_TYPE_LONG;
  bind[0].buffer = (char *) &game.server.fcdb.id;
  bind[0].is_null = 0;
  bind[0].length = 0;

  bind[1].buffer_type = MYSQL_TYPE_BLOB;
  bind[1].buffer = tmap_comp;
  bind[1].buffer_length = len_comp_max;
  bind[1].is_null = 0;
  bind[1].length = &len_comp;

  if (mysql_stmt_bind_param(stmt, bind)) {
    fcdb_error("mysql_stmt_bind_param() failed: %s.",
               mysql_stmt_error(stmt));
    goto ERROR;
  }

  if (mysql_stmt_execute(stmt)) {
    fcdb_error("mysql_stmt_execute() failed: %s.",
               mysql_stmt_error(stmt));
    goto ERROR;
  }

  mysql_stmt_close(stmt);
  free(tmap_comp);

  if (game.server.fcdb.termap) {
    free(game.server.fcdb.termap);
  }
  game.server.fcdb.termap = tmap;

  return TRUE;

ERROR:
  if (stmt) {
    mysql_stmt_close(stmt);
  }
  if (tmap) {
    free(tmap);
  }
  if (tmap_comp) {
    free(tmap_comp);
  }
  if (sock) {
    fcdb_close(sock);
  }
  return FALSE;
}

/**************************************************************************
  ...
**************************************************************************/
static const char *get_termap_rdict(const char *termap)
{
  return termap + FCDB_ETM_HEADER_LEN + FCDB_ETM_DICT_LEN;
}

/**************************************************************************
  ...
**************************************************************************/
static bool fcdb_insert_turn_map(MYSQL *sock, int turn_id)
{
  MYSQL_BIND bind[2];
  MYSQL_STMT *stmt = NULL;
  char *tmap = NULL, *tmap_comp = NULL;
  unsigned long len, len_comp, len_comp_max;
  struct tile_list *changed_list = NULL;
  const char *rdict;
  char enc, buf[256];
  unsigned long index;

  static const char *INSERT_STMT
    = "INSERT INTO turn_maps (turn_id, map) VALUES (?, ?)";

  if (!srvarg.fcdb.save_maps) {
    return TRUE;
  }

  changed_list = tile_list_new();
  tmap = etm_encode_turn_map(game.server.fcdb.termap, changed_list);
  if (!tmap) {
    fcdb_error("Failed to encode turn map.");
    goto ERROR;
  }

  len = get_turnmap_len();
  len_comp_max = 1001 * len / 1000 + 64;
  tmap_comp = fc_malloc(len_comp_max);

  len_comp = len_comp_max; /* Satisfy zlib. */

  switch (compress((unsigned char *)tmap_comp, &len_comp,
                   (unsigned char *)tmap, len)) {
  case Z_OK:
    break;
  case Z_MEM_ERROR:
    fcdb_error("Failed to compress encoded turn map: "
               "not enough memory.");
    goto ERROR;
    break;
  case Z_BUF_ERROR:
    fcdb_error("Failed to compress encoded turn map: "
               "output buffer size exceeded.");
    goto ERROR;
    break;
  default:
    assert(0);
    break;
  }

  freelog(LOG_VERBOSE, "FCDB: Encoded turn map compressed to "
          "%lu bytes down from %lu bytes.",
          len_comp, len);

  stmt = mysql_stmt_init(sock);
  if (!stmt) {
    fcdb_error("mysql_stmt_init() failed: out of memory.");
    goto ERROR;
  }

  if (mysql_stmt_prepare(stmt, INSERT_STMT, strlen(INSERT_STMT))) {
    fcdb_error("mysql_stmt_prepare() failed: %s.",
               mysql_stmt_error(stmt));
    goto ERROR;
  }

  memset(bind, 0, sizeof(bind));

  bind[0].buffer_type = MYSQL_TYPE_LONG;
  bind[0].buffer = (char *) &turn_id;
  bind[0].is_null = 0;
  bind[0].length = 0;

  bind[1].buffer_type = MYSQL_TYPE_BLOB;
  bind[1].buffer = tmap_comp;
  bind[1].buffer_length = len_comp_max;
  bind[1].is_null = 0;
  bind[1].length = &len_comp;

  if (mysql_stmt_bind_param(stmt, bind)) {
    fcdb_error("mysql_stmt_bind_param() failed: %s.",
               mysql_stmt_error(stmt));
    goto ERROR;
  }

  if (mysql_stmt_execute(stmt)) {
    fcdb_error("mysql_stmt_execute() failed: %s.",
               mysql_stmt_error(stmt));
    goto ERROR;
  }

  mysql_stmt_close(stmt);
  free(tmap);
  free(tmap_comp);

  /* Now deal with any terrain that has changed from
   * the original map. */

  rdict = get_termap_rdict(game.server.fcdb.termap);

  tile_list_iterate(changed_list, ptile) {
    index = get_tile_termap_index(ptile);
    enc = etm_encode_terrain(ptile, rdict);
    my_snprintf(buf, sizeof(buf), "INSERT INTO changed_terrain "
                "(turn_id, terrain_map_index, encoded_value) "
                "VALUES (%d, %lu, %d)",
                turn_id, index, enc);
    fcdb_execute_or_return(sock, buf, FALSE);
  } tile_list_iterate_end;

  return TRUE;

ERROR:
  if (stmt) {
    mysql_stmt_close(stmt);
  }
  if (tmap) {
    free(tmap);
  }
  if (tmap_comp) {
    free(tmap_comp);
  }
  if (sock) {
    fcdb_close(sock);
  }
  return FALSE;
}

/**************************************************************************
  ...
**************************************************************************/
static unsigned long get_termap_len(void)
{
  return FCDB_ETM_HEADER_LEN
    + FCDB_ETM_DICT_LEN
    + FCDB_ETM_RDICT_LEN
    + map_num_tiles();
}

/**************************************************************************
  ...
**************************************************************************/
static char etm_encode_terrain(const struct tile *ptile,
                               const char *rdict)
{
  char enc;

  enc = rdict[(unsigned char) get_tile_type(ptile->terrain)->identifier];
  if (tile_has_special(ptile, S_SPECIAL_1)) {
    enc |= FCDB_ETM_SPECIAL1_FLAG;
  }
  if (tile_has_special(ptile, S_SPECIAL_2)) {
    enc |= FCDB_ETM_SPECIAL2_FLAG;
  }
  if (tile_has_special(ptile, S_RIVER)) {
    enc |= FCDB_ETM_RIVER_FLAG;
  }

  return enc;
}

/**************************************************************************
  ...
**************************************************************************/
static char *get_termap_dict(char *termap)
{
  return termap + FCDB_ETM_HEADER_LEN;
}
/**************************************************************************
  ...
**************************************************************************/
static void init_etm_dict(char *termap)
{
  int i, j;
  char ttid, *dict;

  dict = get_termap_dict(termap);

  for (i = 0, j = T_FIRST; i < FCDB_ETM_DICT_LEN && j < T_COUNT; i++) {
    do {
      ttid = get_tile_type(j)->identifier;
      j++;
      if (ttid != 0) {
        break;
      }
    } while (j < T_COUNT);
    dict[i] = ttid;
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void init_etm_rdict(char *termap)
{
  int i;
  const char *dict;
  char *rdict;

  dict = get_termap_dict(termap);

  /* Get rid of constness just this once so we can
   * initialize the reverse dictionary. */
  rdict = (char *) get_termap_rdict(termap);

  for (i = 0; i < FCDB_ETM_DICT_LEN && dict[i] != 0; i++) {
    rdict[(unsigned char) dict[i]] = (char) i;
  }
}

/**************************************************************************
  ...
**************************************************************************/
static unsigned long get_tile_termap_index(const struct tile *ptile)
{
  return FCDB_ETM_HEADER_LEN
      + FCDB_ETM_DICT_LEN
      + FCDB_ETM_RDICT_LEN
      + ptile->index;
}
/**************************************************************************
  ...
**************************************************************************/
static char *etm_encode_terrain_map(void)
{
  int len, i, j, index;
  char *termap;
  const char *rdict;
  struct tile *ptile;

  if (game.ruleset_control.terrain_count > FCDB_ETM_DICT_LEN) {
    freelog(LOG_ERROR, _("Cannot encode more than %d terrain types! "
                         "Ruleset has %d."),
            FCDB_ETM_DICT_LEN, game.ruleset_control.terrain_count);
    return NULL;
  }
  if (map.xsize > 255 || map.ysize > 255) {
    freelog(LOG_ERROR, _("Cannot encode map with dimensions %dx%d: "
                         "too large!"),
            map.xsize, map.ysize);
    return NULL;
  }

  len = get_termap_len();
  termap = fc_calloc(1, len);

  termap[0] = FCDB_ETM_FILE_IDENTIFIER;
  termap[1] = FCDB_ETM_VERSION;
  termap[2] = map.xsize;
  termap[3] = map.ysize;

  init_etm_dict(termap);
  init_etm_rdict(termap);

  rdict = get_termap_rdict(termap);

  for (j = 0; j < map.ysize; j++) {
    for (i = 0; i < map.xsize; i++) {
      ptile = native_pos_to_tile(i, j);
      index = get_tile_termap_index(ptile);
      termap[index] = etm_encode_terrain(ptile, rdict);
    }
  }

  return termap;
}

/**************************************************************************
  ...
**************************************************************************/
static unsigned long get_turnmap_len(void)
{
  return map_num_tiles();
}

/**************************************************************************
  ...
**************************************************************************/
static char *etm_encode_turn_map(const char *termap,
                                 struct tile_list *changed_terrain_list)
{
  struct tile *ptile;
  int i, j, len, index;
  char enc, *turnmap, tenc;
  const char *rdict;

  if (!termap) {
    return NULL;
  }

  len = get_turnmap_len();
  turnmap = fc_calloc(1, len);

  rdict = get_termap_rdict(termap);

  for (j = 0; j < map.ysize; j++) {
    for (i = 0; i < map.xsize; i++) {
      ptile = native_pos_to_tile(i, j);
      if (ptile->city) {
        enc = city_owner(ptile->city)->player_no;
        enc |= FCDB_ETM_CITY_FLAG;
      } else if (unit_list_size(ptile->units) > 0) {
        enc = unit_owner(unit_list_get(ptile->units, 0))->player_no;
        enc |= FCDB_ETM_UNIT_FLAG;
      } else {
        enc = 0;
      }

      tenc = etm_encode_terrain(ptile, rdict);
      index = get_tile_termap_index(ptile);
      if (tenc != termap[index]) {
        enc |= FCDB_ETM_CHANGED_TERRAIN_FLAG;
        if (changed_terrain_list) {
          tile_list_append(changed_terrain_list, ptile);
        }
      }

      index = native_pos_to_index(i, j);
      turnmap[index] = enc;
    }
  }

  return turnmap;
}

/**************************************************************************
  Loads the player's rating for this type of game. If the no ratings
  exist for the give user_id and game type, assigns (but does not save
  in the database) a new rating.
**************************************************************************/
static bool load_old_rating(MYSQL *sock,
                            int user_id,
                            struct player *pplayer,
                            int game_type)
{
  MYSQL_RES *res = NULL;
  char buf[1024];

  my_snprintf(buf, sizeof(buf),
      "SELECT rating, rating_deviation, UNIX_TIMESTAMP(timestamp)"
      "  FROM ratings"
      "  WHERE id = ("
      "    SELECT MAX(id) FROM ratings"
      "      WHERE user_id = %d AND game_type = '%s')",
      user_id, game_type_name_orig(game_type));
  fcdb_execute_or_return(sock, buf, FALSE);

  res = mysql_store_result(sock);

  if (mysql_num_rows(res) == 0) {
    score_assign_new_player_rating(pplayer, game_type);

  } else if (mysql_num_rows(res) == 1) {
    MYSQL_ROW row = mysql_fetch_row(res);
    pplayer->fcdb.rating = atof(row[0]);
    pplayer->fcdb.rating_deviation = atof(row[1]);
    pplayer->fcdb.last_rating_timestamp = (time_t) atol(row[2]);
  } else {
    /* This should not happen... */
    fcdb_error("Unexpected number of rows (%d) for query:\n%s",
               mysql_num_rows(res), buf);
    mysql_free_result(res);
    fcdb_close(sock);
    return FALSE;
  }

  mysql_free_result(res);
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool get_user_id(MYSQL *sock, const char *user_name, int *id)
{
  MYSQL_RES *res;
  char buf[1024];

  *id = 0;

  my_snprintf(buf, sizeof(buf),
      "SELECT id FROM %s WHERE name = '%s'",
      AUTH_TABLE, fcdb_escape(sock, user_name));
  fcdb_reset_escape_buffer();
  fcdb_execute_or_return(sock, buf, FALSE);

  res = mysql_store_result(sock);
  if (mysql_num_rows(res) == 1) {
    MYSQL_ROW row = mysql_fetch_row(res);
    *id = atoi(row[0]);
  }
  mysql_free_result(res);

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool reload_termap()
{
  MYSQL mysql, *sock = &mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  char buf[1024];
  unsigned long *lengths, len;

  if (game.server.fcdb.termap != NULL) {
    return TRUE;
  }

  if (!fcdb_connect(sock)) {
    return FALSE;
  }

  my_snprintf(buf, sizeof(buf), "SELECT map FROM terrain_maps "
              "WHERE game_id = %d", game.server.fcdb.id);
  fcdb_execute_or_return(sock, buf, FALSE);
  
  res = mysql_store_result(sock);
  if (mysql_num_rows(res) == 1) {
    bool succeeded = FALSE;

    row = mysql_fetch_row(res);
    lengths = mysql_fetch_lengths(res);
    len = get_termap_len();
    game.server.fcdb.termap = fc_malloc(len);

    switch (uncompress((unsigned char *)game.server.fcdb.termap, &len,
                       (unsigned char *)row[0], lengths[0])) {
    case Z_MEM_ERROR:
      freelog(LOG_ERROR, "FCDB: Failed to uncompress reloaded "
              "terrain map: out of memory.");
      break;
    case Z_BUF_ERROR:
      freelog(LOG_ERROR, "FCDB: Failed to uncompress reloaded "
              "terrain map: destination buffer too small!.");
      break;
    case Z_DATA_ERROR:
      freelog(LOG_ERROR, "FCDB: Failed to uncompress reloaded "
              "terrain map: compressed data is corrupted.");
      break;
    case Z_OK:
      succeeded = TRUE;
      break;

    default:
      /* Must not happen. */
      assert(FALSE);
      break;
    }

    if (!succeeded) {
      free(game.server.fcdb.termap);
      game.server.fcdb.termap = NULL;
    }
  }
  mysql_free_result(res);

  if (game.server.fcdb.termap == NULL) {
    /* We might as well insert a new one then... */
    if (!fcdb_insert_terrain_map(sock)) {
      return FALSE;
    }
  }

  fcdb_close(sock);

  return TRUE;
}
#endif /* HAVE_MYSQL */

/**************************************************************************
  Called once at the start of game to make the initial game record in the
  database and record the players in this game. Assumes game_set_type()
  has already been called.
**************************************************************************/
bool fcdb_record_game_start(void)
{
#ifdef HAVE_MYSQL
  MYSQL mysql, *sock = &mysql;
  char buf[1024], host[512];
  struct settings_s *op;

  if (!srvarg.fcdb.enabled) {
    return TRUE;
  }

  /* Check if this is a reloaded game. If so, we don't
   * want to overwrite the existing start data. */
  if (game.server.fcdb.id > 0) {
    freelog(LOG_VERBOSE, _("FCDB: Game %d resumed."), game.server.fcdb.id);

    /* We might have to recreate the termap... */
    return reload_termap();
  }

  freelog(LOG_VERBOSE, _("FCDB: Recording game start into database."));

  if (!fcdb_connect(sock)) {
    return FALSE;
  }

  if (my_gethostname(host, sizeof(host)) != 0) {
    sz_strlcpy(host, "unknown");
  }

  /* The game record. */
  my_snprintf(buf, sizeof(buf), "INSERT INTO games"
      "(host, port, version, patches, capabilities, "
      "ruleset, type)"
      "VALUES ('%s', '%d', '%s', '%s', '%s', '%s', '%s')",
      host, srvarg.port, fcdb_escape(sock, VERSION_STRING),
      fcdb_escape(sock, get_meta_patches_string()),
      fcdb_escape(sock, our_capability),
      fcdb_escape(sock, game.server.rulesetdir),
      game_type_name_orig(game.server.fcdb.type));
  fcdb_reset_escape_buffer();
  fcdb_execute_or_return(sock, buf, FALSE);
  game.server.fcdb.id = mysql_insert_id(sock);

  /* Record the terrain map. */
  if (!fcdb_insert_terrain_map(sock)) {
    return FALSE;
  }

  /* Record any non default settings. */
  for (op = settings; op->name; op++) {
    
    /* Skip if at the default value. */
    if ((op->type == SSET_BOOL
         && *op->bool_value == op->bool_default_value)
        || (op->type == SSET_INT
            && *op->int_value == op->int_default_value)
        || (op->type == SSET_STRING
            && 0 == strcmp(op->string_value, op->string_default_value))) {
      continue;
    }
    
    switch (op->type) {
    case SSET_BOOL:
      my_snprintf(buf, sizeof(buf), "INSERT INTO non_default_settings"
          "(game_id, name, value) VALUES (%d, '%s', '%s')",
          game.server.fcdb.id, op->name, *op->bool_value ? "true" : "false");
      break;
    case SSET_INT:
      my_snprintf(buf, sizeof(buf), "INSERT INTO non_default_settings"
          "(game_id, name, value) VALUES (%d, '%s', '%d')",
          game.server.fcdb.id, op->name, *op->int_value);
      break;
    case SSET_STRING:
      my_snprintf(buf, sizeof(buf), "INSERT INTO non_default_settings"
          "(game_id, name, value) VALUES (%d, '%s', '%s')",
          game.server.fcdb.id, op->name, fcdb_escape(sock, op->string_value));
      fcdb_reset_escape_buffer(); 
      break;
    default:
      assert(0);
      break;
    }
    fcdb_execute_or_return(sock, buf, FALSE);
  }

  /* Record the teams, if any. */
  team_iterate(pteam) {
    my_snprintf(buf, sizeof(buf), "INSERT INTO teams "
        "(game_id, name, num_players) VALUES (%d, '%s', %d)",
        game.server.fcdb.id,
        fcdb_escape(sock, get_team_name(pteam->id)),
        pteam->member_count);
    fcdb_reset_escape_buffer();
    fcdb_execute_or_return(sock, buf, FALSE);
  } team_iterate_end;

  /* Record the initial players. */
  players_iterate(pplayer) {
    if (is_barbarian(pplayer)) {
      continue;
    }
    if (!fcdb_insert_player(sock, pplayer)) {
      return FALSE;
    }
    player_setup_turns_played(pplayer);
  } players_iterate_end;

  fcdb_commit(sock);
  fcdb_close(sock);
#endif

  return TRUE;
}

/**************************************************************************
  Called once at the end of turn to update game status, turn information,
  and player statuses for that turn.
**************************************************************************/
bool fcdb_end_of_turn_update(void)
{
#ifdef HAVE_MYSQL
  MYSQL mysql, *sock = &mysql;
  MYSQL_RES *res;
  char buf[4096];
  int turn_id, lux_income, num;

  if (!srvarg.fcdb.enabled) {
    return TRUE;
  }

  freelog(LOG_VERBOSE, _("FCDB: Recording turn data into database."));

  if (game.server.fcdb.id <= 0) {
    fcdb_error("Current game_id is invalid.");
    return FALSE;
  }

  if (!fcdb_connect(sock)) {
    return FALSE;
  }

  /* Check that there isn't already a record for this turn
   * (e.g. if this is a replayed loaded game). */
  my_snprintf(buf, sizeof(buf), "SELECT id FROM turns "
      "WHERE game_id = %d AND turn_no = %d",
      game.server.fcdb.id, game.info.turn);
  fcdb_execute_or_return(sock, buf, FALSE);
  res = mysql_store_result(sock);
  num = mysql_num_rows(res);
  mysql_free_result(res);
  if (num > 0) {
    fcdb_error("Turn %d for game %d already in database!",
               game.info.turn, game.server.fcdb.id);
    notify_conn(NULL, _("Server: Database updating disabled "
                "because this game is being replayed from "
                "a previously recorded point."));
    fcdb_close(sock);
    return FALSE;
  }

  /* Make a record for this turn. */
  my_snprintf(buf, sizeof(buf), "INSERT INTO turns"
      "(game_id, turn_no, year) VALUES (%d, %d, %d)",
      game.server.fcdb.id, game.info.turn, game.info.year);
  fcdb_execute_or_return(sock, buf, FALSE);

  turn_id = mysql_insert_id(sock);

  if (!fcdb_insert_turn_map(sock, turn_id)) {
    return FALSE;
  }
  
  /* Update the game record with that last turn we know about. */
  my_snprintf(buf, sizeof(buf), "UPDATE games "
      "SET last_turn_id = %d, last_update = NOW() WHERE id = %d",
      turn_id, game.server.fcdb.id);
  fcdb_execute_or_return(sock, buf, FALSE);

  /* Check to see if any new players have magically appeared. */
  players_iterate(pplayer) {
    if (is_barbarian(pplayer) || pplayer->fcdb.player_id > 0) {
      continue;
    }
    if (!fcdb_insert_player(sock, pplayer)) {
      return FALSE;
    }
  } players_iterate_end;


  /* Make a status record for each player for this turn. */
  players_iterate(pplayer) {
    int player_status_id, i;
    struct player *pplayer_ally;

    if (is_barbarian(pplayer)) {
      continue;
    }

    lux_income = 0;
    city_list_iterate(pplayer->cities, pcity) {
      lux_income += pcity->luxury_total;
    } city_list_iterate_end;

    /* Give my_snprintf a workout. */
    my_snprintf(buf, sizeof(buf), "INSERT INTO player_status "
        "(turn_id, player_id, alive, score, "
        "gold, techs, gold_income, science_income, "
        "luxury_income, trade_production, shield_production, "
        "population, happy_citizens, content_citizens, "
        "unhappy_citizens, angry_citizens, taxmen, "
        "scientists, entertainers, cities, settledarea, "
        "landarea, pollution, military_units, civilian_units, "
        "literacy, wonders, government, tax_rate, "
        "science_rate, luxury_rate) "
        "VALUES (%d, %d, %s, %d, "
        "%d, %d, %d, %d, "
        "%d, %d, %d, "
        "%d, %d, %d, "
        "%d, %d, %d, "
        "%d, %d, %d, %d, "
        "%d, %d, %d, %d, "
        "%d, %d, '%s', %d, "
        "%d, %d)",
        turn_id, pplayer->fcdb.player_id,
        pplayer->is_alive ? "TRUE" : "FALSE",
        get_civ_score(pplayer),

        pplayer->economic.gold, pplayer->score.techs,
        player_get_expected_income(pplayer), pplayer->score.techout,

        lux_income, pplayer->score.bnp, pplayer->score.mfg,

        pplayer->score.population * 1000, pplayer->score.happy,
        pplayer->score.content,

        pplayer->score.unhappy, pplayer->score.angry,
        pplayer->score.taxmen,

        pplayer->score.scientists, pplayer->score.elvis,
        pplayer->score.cities, pplayer->score.settledarea,
        
        pplayer->score.landarea, pplayer->score.pollution,
        pplayer->score.units,
        unit_list_size(pplayer->units) - pplayer->score.units,

        pplayer->score.literacy, pplayer->score.wonders,
        fcdb_escape(sock, get_gov_pplayer(pplayer)->name_orig),
        pplayer->economic.tax,

        pplayer->economic.science, pplayer->economic.luxury);

    fcdb_reset_escape_buffer();
    fcdb_execute_or_return(sock, buf, FALSE);
    player_status_id = mysql_insert_id(sock);
    
    /* Record controlling user if any. */
    if (pplayer->is_connected
        && 0 != strcmp(pplayer->username, ANON_USER_NAME)) {
      int turns;

      my_snprintf(buf, sizeof(buf), "UPDATE player_status "
          "SET user_name = '%s' WHERE id = %d",
          fcdb_escape(sock, pplayer->username),
          player_status_id);
      fcdb_reset_escape_buffer();
      fcdb_execute_or_return(sock, buf, FALSE);

      turns = player_get_turns_played(pplayer, pplayer->username);
      player_set_turns_played(pplayer, pplayer->username, turns + 1);
    }

    /* Record whether the player is controlled by an ai. */
    if (pplayer->ai.control) {
      my_snprintf(buf, sizeof(buf), "UPDATE player_status "
          "SET ai_mode = '%s' WHERE id = %d",
          name_of_skill_level(pplayer->ai.skill_level),
          player_status_id);
      fcdb_execute_or_return(sock, buf, FALSE);
    }

    /* Record allies this turn. */
    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      if (i == pplayer->player_no
          || pplayer->diplstates[i].type != DS_ALLIANCE) {
        continue;
      }
      pplayer_ally = get_player(i);
      my_snprintf(buf, sizeof(buf), "INSERT INTO allies "
          "(player_status_id, ally_player_id) VALUES (%d, %d)",
          player_status_id, pplayer_ally->fcdb.player_id);
      fcdb_execute_or_return(sock, buf, FALSE);
    }

  } players_iterate_end;
  
  fcdb_commit(sock);
  fcdb_close(sock);
#endif

  return TRUE;
}

/**************************************************************************
  Called once at the end of game to update the database with the final
  state of the game.
**************************************************************************/
bool fcdb_record_game_end(void)
{
#ifdef HAVE_MYSQL
  MYSQL mysql, *sock = &mysql;
  char buf[1024];

  if (!srvarg.fcdb.enabled) {
    return TRUE;
  }

  freelog(LOG_VERBOSE, _("FCDB: Recording game end into database."));

  if (game.server.fcdb.id <= 0) {
    fcdb_error("Current game_id is invalid.");
    return FALSE;
  }

  if (!fcdb_connect(sock)) {
    return FALSE;
  }


  /* Record player results and ranks. */
  players_iterate(pplayer) {
    if (pplayer->fcdb.player_id <= 0) {
      continue;
    }
    my_snprintf(buf, sizeof(buf), "UPDATE players "
        "SET result = '%s', rank = %f WHERE id = %d",
        result_name_orig(pplayer->result),
        pplayer->rank,
        pplayer->fcdb.player_id);
    fcdb_execute_or_return(sock, buf, FALSE);
    player_free_turns_played(pplayer);
  } players_iterate_end;

  /* Record team results and ranks. */
  team_iterate(pteam) {
    my_snprintf(buf, sizeof(buf), "UPDATE teams "
        "SET score = %f, rank = %f, result = '%s'"
        "WHERE game_id = %d AND STRCMP(name, '%s') = 0",
        pteam->score, pteam->rank, result_name_orig(pteam->result),
        game.server.fcdb.id,
        fcdb_escape(sock, get_team_name(pteam->id)));
    fcdb_reset_escape_buffer();
    fcdb_execute_or_return(sock, buf, FALSE);
  } team_iterate_end;

  /* Record game outcome. */
  my_snprintf(buf, sizeof(buf), "UPDATE games "
      "SET outcome = '%s' WHERE id = %d",
      game_outcome_name_orig(game.server.fcdb.outcome), game.server.fcdb.id);
  fcdb_execute_or_return(sock, buf, FALSE);

  if (game.server.rated) {
    players_iterate(pplayer) {
      if (pplayer->fcdb.player_id <= 0
          || pplayer->fcdb.rated_user_id <= 0
          || pplayer->fcdb.new_rating == 0.0
          || pplayer->fcdb.new_rating_deviation == 0.0) {
        continue;
      }
      my_snprintf(buf, sizeof(buf), "INSERT INTO ratings "
          "(user_id, game_id, player_id,"
          " old_rating, old_rating_deviation,"
          " rating, rating_deviation, game_type) "
          "VALUES (%d, %d, %d, %f, %f, %f, %f, '%s')",
          pplayer->fcdb.rated_user_id,
          game.server.fcdb.id,
          pplayer->fcdb.player_id,
          pplayer->fcdb.rating,
          pplayer->fcdb.rating_deviation,
          pplayer->fcdb.new_rating,
          pplayer->fcdb.new_rating_deviation,
          game_type_name_orig(game.server.fcdb.type));
      fcdb_execute_or_return(sock, buf, FALSE);
    } players_iterate_end;
  }

  fcdb_commit(sock);
  fcdb_close(sock);
#endif

  return TRUE;
}

/**************************************************************************
  Fills player fcdb struct with rating information for the given game
  type. May be called in pregame or during the game. If the boolean
  argument 'check_turns_played' is true, then ratings will be loaded
  taking into account which user played the most significantly for
  each given player.

  NB: This function modifies the fcdb fields of the player structs.
**************************************************************************/
bool fcdb_load_player_ratings(int game_type, bool check_turns_played)
{
#ifdef HAVE_MYSQL
  MYSQL mysql, *sock = &mysql;
  char buf[1024];
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  int user_id_for_old_rating;
  bool rate_user;

  freelog(LOG_DEBUG, "Loading player ratings from database...");

  fcdb_connect_or_return(sock, FALSE);

  players_iterate(pplayer) {
    if (pplayer->is_civil_war_split || is_barbarian(pplayer)) {
      continue;
    }

    /* Whether the user should get a rating update.
     * False for AIs. */
    rate_user = TRUE;

    /* The user id from which to get the old rating of
     * this player. */
    user_id_for_old_rating = 0;

    if (check_turns_played) {
      int most_turns = 0;
      const char *most_user = NULL;

      player_turns_played_iterate(pplayer, username, turns) {
        if (turns > most_turns) {
          most_turns = turns;
          most_user = username;
        }
      } player_turns_played_iterate_end;

      if (!most_user) {
        rate_user = FALSE;
      }

      /* The following check is only performed if enough turns
       * have been played in the game for the game to be rated
       * at all (if not, the function game_can_be_rated in
       * score.c will handle the check). */
      if (rate_user && game.info.turn >= srvarg.fcdb.min_rated_turns
          && most_turns < srvarg.fcdb.min_rated_turns) {
        notify_conn(NULL, _("Server: User %s will not receive an updated "
                            "rating because not enough turns were played "
                            "(only played %d, which is less than the "
                            "minimum of %d)."),
                    most_user, most_turns, srvarg.fcdb.min_rated_turns);
        rate_user = FALSE;
      }

      if (!get_user_id(sock, most_user, &user_id_for_old_rating)) {
        rate_user = FALSE;
        user_id_for_old_rating = 0;
      }
    } else if (pplayer->is_connected) {
      /* I assume is_connected == TRUE guarantees that
       * the user_name field is an actual user name. */
      if (!get_user_id(sock, pplayer->username,
                       &user_id_for_old_rating)) {
        goto FAILED;
      }
    } else {
      /* No user connected, assume it is an AI player. */
      user_id_for_old_rating = 0;
      rate_user = FALSE;
    }

    freelog(LOG_DEBUG, "pplayer %p \"%s\" user_id_for_old_rating=%d "
            "rate_user=%d",
            pplayer, pplayer->name, user_id_for_old_rating, rate_user);

    if (user_id_for_old_rating > 0) {
      if (!load_old_rating(sock, user_id_for_old_rating,
                           pplayer, game_type)) {
        goto FAILED;
      }

      if (rate_user) {
        pplayer->fcdb.rated_user_id = user_id_for_old_rating;
        my_snprintf(buf, sizeof(buf),
          "SELECT name FROM %s WHERE id = %d",
          AUTH_TABLE, user_id_for_old_rating);
        if (!fcdb_execute(sock, buf)) {
          goto FAILED;
        }
        res = mysql_store_result(sock);
        if (mysql_num_rows(res) == 1) {
          row = mysql_fetch_row(res);
          if (row[0]) {
            sz_strlcpy(pplayer->fcdb.rated_user_name, row[0]);
          }
        } else {
          pplayer->fcdb.rated_user_name[0] = '\0';
        }
        mysql_free_result(res);
        res = NULL;

      } else {
        pplayer->fcdb.rated_user_id = 0;
        pplayer->fcdb.rated_user_name[0] = '\0';
      }
    } else {
      score_assign_ai_rating(pplayer, game_type);
    }
  } players_iterate_end;
  
  fcdb_close(sock);
  return TRUE;


FAILED:

  /* Make sure nobody gets a rating update if
   * we encounter some errors. */
  players_iterate(pplayer) {
    pplayer->fcdb.rating = 0;
    pplayer->fcdb.rating_deviation = 0;
    pplayer->fcdb.rated_user_id = 0;
    pplayer->fcdb.rated_user_name[0] = '\0';
  } players_iterate_end;

  if (res) {
    mysql_free_result(res);
  }

  /* Error code already closed 'sock'. */

#endif /* HAVE_MYSQL */
  return FALSE;
}

/**************************************************************************
  Fill the stats struct with statistics about the given user. Returns
  NULL on error. If the username is unknown, the 'id' field will be <= 0.
  Call fcdb_user_stats_free when done with the returned pointer.

  If 'matchs' is not NULL and the given username matchs multiple
  users when used in a pattern as '%username%' (i.e. using MySQL
  pattern semantics), then it is filled with at most 50 matching
  names.

  NB: The caller must free the newly allocated strings inside 'matchs'
  when done.
**************************************************************************/
struct fcdb_user_stats *fcdb_user_stats_new(const char *username,
                                            struct string_list *matchs)
{
#ifdef HAVE_MYSQL
  MYSQL mysql, *sock = &mysql;
  char buf[1024];
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  struct fcdb_user_stats *fus = NULL;
  int i, j, num_rows;

  fcdb_connect_or_return(sock, NULL);

  fus = fc_calloc(1, sizeof(struct fcdb_user_stats));
  
  if (username == NULL || username[0] == '\0') {
    /* Don't bother searching for an empty user name,
     * just say that we don't know any like that. ;) */
    fcdb_close(sock);
    return fus;    
  }

  if (!get_user_id(sock, username, &fus->id)) {
    goto ERROR;
  }

  if (fus->id <= 0) {
    char pattern[MAX_LEN_NAME * 2 + 16];
    int len = strlen(username);

    for (i = 0, j = 0; i < len && j < sizeof(pattern) - 2; i++, j++) {
      if (username[i] == '%' || username[i] == '_') {
        pattern[j++] = '\\';
      }
      pattern[j] = username[i];
    }
    pattern[j] = '\0';

    my_snprintf(buf, sizeof(buf),
        "SELECT id, name FROM %s "
        "  WHERE name IS NOT NULL "
        "  AND name LIKE '%%%s%%' LIMIT 50",
        AUTH_TABLE, fcdb_escape(sock, pattern));
    fcdb_reset_escape_buffer();
    if (!fcdb_execute(sock, buf)) {
      goto ERROR;
    }

    res = mysql_store_result(sock);
    num_rows = mysql_num_rows(res);

    if (num_rows != 1) {
      for (i = 0; i < num_rows; i++) {
        row = mysql_fetch_row(res);
        if (matchs != NULL) {
          string_list_append(matchs, mystrdup(row[1]));
        }
      }
    } else {
      row = mysql_fetch_row(res);
      fus->id = atoi(row[0]);
    }

    mysql_free_result(res);
    
    if (num_rows != 1) {
      fcdb_close(sock);
      return fus;
    }
  }

  my_snprintf(buf, sizeof(buf), "SELECT name, email, createtime, "
      "accesstime, address, createaddress, logincount FROM %s "
      "WHERE id = '%d'", AUTH_TABLE, fus->id);
  if (!fcdb_execute(sock, buf)) {
    goto ERROR;
  }

  res = mysql_store_result(sock);
  row = mysql_fetch_row(res);

  sz_strlcpy(fus->username, row[0]);
  if (row[1]) {
    sz_strlcpy(fus->email, row[1]);
  }
  fus->createtime = (time_t) atol(row[2]);
  fus->accesstime = (time_t) atol(row[3]);
  sz_strlcpy(fus->address, row[4]);
  sz_strlcpy(fus->createaddress, row[5]);
  fus->logincount = atoi(row[6]);

  mysql_free_result(res);
  res = NULL;

  my_snprintf(buf, sizeof(buf),
      "SELECT game_type, COUNT(game_type) FROM ratings"
      "  WHERE user_id = %d"
      "  GROUP BY game_type",
      fus->id);
  if (!fcdb_execute(sock, buf)) {
    goto ERROR;
  }

  res = mysql_store_result(sock);
  fus->num_game_types = mysql_num_rows(res);
  if (fus->num_game_types > 0) {
    fus->gt_stats = fc_calloc(fus->num_game_types,
                              sizeof(struct game_type_stats));
    for (i = 0; i < fus->num_game_types; i++) {
      row = mysql_fetch_row(res);
      sz_strlcpy(fus->gt_stats[i].type, row[0]);
      fus->gt_stats[i].count = atoi(row[1]);
    }
  }
  mysql_free_result(res);
  res = NULL;

  for (i = 0; i < fus->num_game_types; i++) {
    my_snprintf(buf, sizeof(buf),
      "SELECT p.result, COUNT(p.result) "
      "  FROM ratings AS r INNER JOIN players AS p"
      "  ON r.player_id = p.id"
      "  WHERE r.user_id = %d AND r.game_type = '%s'"
      "  GROUP BY p.result",
      fus->id, fus->gt_stats[i].type);

    if (!fcdb_execute(sock, buf)) {
      goto ERROR;
    }

    res = mysql_store_result(sock);
    num_rows = mysql_num_rows(res);
    for (j = 0; j < num_rows; j++) {
      row = mysql_fetch_row(res);
      if (0 == strcmp(row[0], result_name_orig(PR_WIN))) {
        fus->gt_stats[i].wins = atoi(row[1]);
      } else if (0 == strcmp(row[0], result_name_orig(PR_LOSE))) {
        fus->gt_stats[i].loses = atoi(row[1]);
      } else if (0 == strcmp(row[0], result_name_orig(PR_DRAW))) {
        fus->gt_stats[i].draws = atoi(row[1]);
      }
    }
    mysql_free_result(res);
    res = NULL;

    my_snprintf(buf, sizeof(buf),
      "SELECT rating, rating_deviation FROM ratings"
      "  WHERE id = ("
      "    SELECT MAX(id) FROM ratings"
      "      WHERE user_id = %d AND game_type = '%s')",
      fus->id, fus->gt_stats[i].type);
    if (!fcdb_execute(sock, buf)) {
      goto ERROR;
    }

    res = mysql_store_result(sock);
    if (mysql_num_rows(res) == 1) {
      row = mysql_fetch_row(res);
      fus->gt_stats[i].rating = atof(row[0]);
      fus->gt_stats[i].rating_deviation = atof(row[1]);
    }
    mysql_free_result(res);
    res = NULL;
  }

  fcdb_close(sock);

  return fus;

ERROR:
  fcdb_user_stats_free(fus);
  if (res) {
    mysql_free_result(res);
  }
  return NULL;

#else
  /* Should not really happen, but just in case. */
  return NULL;
#endif /* HAVE_MYSQL */
}

/**************************************************************************
  ...
**************************************************************************/
void fcdb_user_stats_free(struct fcdb_user_stats *fus)
{
  if (fus == NULL) {
    return;
  }
  if (fus->num_game_types > 0 && fus->gt_stats != NULL) {
    free(fus->gt_stats);
    fus->num_game_types = 0;
    fus->gt_stats = NULL;
  }
  free(fus);
}

/**************************************************************************
  Returns a positive integer if the user exists or zero, if not. Returns
  -1 on error.
**************************************************************************/
int fcdb_user_exists(const char *username)
{
#ifdef HAVE_MYSQL
  MYSQL mysql, *sock = &mysql;
  int id = 0;

  fcdb_connect_or_return(sock, 0);

  if (!get_user_id(sock, username, &id)) {
    return -1;
  }

  fcdb_close(sock);

  return id;
#endif

  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
bool fcdb_get_user_rating(const char *username,
                          int game_type,
                          double *prating,
                          double *prating_deviation)
{
#ifdef HAVE_MYSQL
  MYSQL mysql, *sock = &mysql;
  char buf[1024];
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  int id = 0;

  fcdb_connect_or_return(sock, FALSE);

  if (!get_user_id(sock, username, &id)) {
    return FALSE;
  }

  if (id <= 0) {
    *prating = 0.0;
    *prating_deviation = 0.0;
    fcdb_close(sock);
    return TRUE;
  }

  if (game_type_name_orig(game_type) == NULL) {
    game_type = game.server.fcdb.type;
  }

  my_snprintf(buf, sizeof(buf),
      "SELECT rating, rating_deviation FROM ratings"
      "  WHERE id = ("
      "    SELECT MAX(id) FROM ratings"
      "      WHERE user_id = %d AND game_type = '%s')",
      id, game_type_name_orig(game_type));
  fcdb_execute_or_return(sock, buf, FALSE);

  res = mysql_store_result(sock);
  if (mysql_num_rows(res) == 1) {
    row = mysql_fetch_row(res);
    *prating = atof(row[0]);
    *prating_deviation = atof(row[1]);
  } else  {
    *prating = 0.0;
    *prating_deviation = 0.0;
  }
  mysql_free_result(res);


  fcdb_close(sock);

  return TRUE;
#endif
  return FALSE;
}

/**************************************************************************
  ...
**************************************************************************/
bool fcdb_get_recent_games(const char *username,
                           int *recent_games,
                           int *count)
{
#ifdef HAVE_MYSQL
  MYSQL mysql, *sock = &mysql;
  char buf[1024];
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  int id = 0, i;

  fcdb_connect_or_return(sock, FALSE);

  if (!get_user_id(sock, username, &id)) {
    return FALSE;
  }

  if (id <= 0 || *count <= 0) {
    *count = 0;
  } else {
    my_snprintf(buf, sizeof(buf),
        "SELECT g.id FROM games AS g INNER JOIN ratings AS r"
        "  ON g.id = r.game_id"
        "  WHERE r.user_id = %d"
        "  ORDER BY g.id DESC"
        "  LIMIT %d",
        id, *count);
    *count = 0;
    fcdb_execute_or_return(sock, buf, FALSE);

    res = mysql_store_result(sock);
    *count = mysql_num_rows(res);
    for (i = *count - 1; i >= 0; i--) {
      row = mysql_fetch_row(res);
      recent_games[i] = atoi(row[0]);
    }
    mysql_free_result(res);
  }

  fcdb_close(sock);

  return TRUE;
#endif
  *count = 0;
  return TRUE;
}

/**************************************************************************
  The 'id' parameter is the game_id (a.k.a. game #) in the database.
  If the given id does not match any known game then a newly allocated
  struct fcdb_game_info pointer is returned with its id field set to 0.
**************************************************************************/
struct fcdb_game_info *fcdb_game_info_new(int id)
{
#ifdef HAVE_MYSQL
  MYSQL mysql, *sock = &mysql;
  char buf[1024];
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  int i, last_turn_id = 0;
  struct fcdb_game_info *fgi = NULL;
  struct fcdb_player_in_game_info *fpigi;
  struct fcdb_team_in_game_info *ftigi;

  fcdb_connect_or_return(sock, NULL);


  /* Fetch information about the game. */

  my_snprintf(buf, sizeof(buf),
      "SELECT id, host, port, type, outcome, UNIX_TIMESTAMP(created), "
      "    UNIX_TIMESTAMP(last_update), last_turn_id"
      "  FROM games WHERE id = %d",
      id);
  fcdb_execute_or_return(sock, buf, NULL);

  fgi = fc_calloc(1, sizeof(struct fcdb_game_info));
  
  res = mysql_store_result(sock);
  if (mysql_num_rows(res) != 1) {
    mysql_free_result(res);
    fcdb_close(sock);

    /* No game was found, returning with fgi->id set to 0. */
    return fgi;
  }

  row = mysql_fetch_row(res);
  fgi->id = atoi(row[0]);
  sz_strlcpy(fgi->host, row[1] != NULL ? row[1] : _("<unknown>"));
  if (row[2]) {
    fgi->port = atoi(row[2]);
  }
  sz_strlcpy(fgi->type, row[3] != NULL ? row[3] : _("<not set>"));
  sz_strlcpy(fgi->outcome, row[4] != NULL ? row[4] : _("<not set>"));
  fgi->completed = row[4] != NULL;
  fgi->created = atol(row[5]);
  if (row[6]) {
    fgi->duration = atol(row[6]) - fgi->created;
  } else {
    /* For the strange case that the last_update field is NULL.
     * This should not happen, but just in case. */
    fgi->duration = 0;
  }
  if (row[7] != NULL) {
    last_turn_id = atoi(row[7]);
  }

  mysql_free_result(res);
  res = NULL;


  if (last_turn_id > 0) {
    my_snprintf(buf, sizeof(buf),
        "SELECT turn_no FROM turns WHERE id = %d",
        last_turn_id);
    if (!fcdb_execute(sock, buf)) {
      goto ERROR;
    }

    res = mysql_store_result(sock);
    if (mysql_num_rows(res) != 1) {
      fcdb_error(_("Did not get expected turn row from turns table. "
                   "Database may be corrupted."));
      fcdb_close(sock);
      goto ERROR;
    }

    row = mysql_fetch_row(res);
    fgi->num_turns = atoi(row[0]);
    mysql_free_result(res);
    res = NULL;
  }

  
  /* Fetch information about players. */

  my_snprintf(buf, sizeof(buf),
      "SELECT p.name, p.creating_user_name, p.nation, "
      "    t.name, p.result, p.rank"
      "  FROM players AS p LEFT JOIN teams AS t"
      "    ON p.team_id = t.id"
      "  WHERE p.game_id = %d"
      "  ORDER BY p.rank",
      id);
  if (!fcdb_execute(sock, buf)) {
    goto ERROR;
  }

  res = mysql_store_result(sock);
  fgi->num_players = mysql_num_rows(res);
  if (fgi->num_players > 0) {
    fgi->players = fc_calloc(fgi->num_players,
                             sizeof(struct fcdb_player_in_game_info));
    for (i = 0; i < fgi->num_players; i++) {
      row = mysql_fetch_row(res);
      fpigi = &fgi->players[i];
      sz_strlcpy(fpigi->name, row[0]);
      if (row[1]) {
        sz_strlcpy(fpigi->user, row[1]);
      }
      sz_strlcpy(fpigi->nation, row[2]);
      if (row[3]) {
        sz_strlcpy(fpigi->team_name, row[3]);
      }
      if (row[4]) {
        sz_strlcpy(fpigi->result, row[4]);
      }
      if (row[5]) {
        fpigi->rank = atof(row[5]);
      }
    }
  }
  mysql_free_result(res);
  res = NULL;


  if (srvarg.fcdb.more_game_info) {

    /* Fetch the players' final scores. */

    my_snprintf(buf, sizeof(buf),
        "SELECT ps.score FROM games AS g "
        "  INNER JOIN turns AS t "
        "    ON g.id = t.game_id"
        "  INNER JOIN player_status AS ps"
        "    ON t.id = ps.turn_id"
        "  INNER JOIN players AS p"
        "    ON p.id = ps.player_id"
        "  WHERE g.id = %d"
        "    AND g.last_turn_id IS NOT NULL"
        "    AND t.id = g.last_turn_id"
        "  ORDER BY p.rank",
        id);
    if (!fcdb_execute(sock, buf)) {
      goto ERROR;
    }

    res = mysql_store_result(sock);
    if (mysql_num_rows(res) != fgi->num_players) {
      goto ERROR;
    }

    for (i = 0; i < fgi->num_players; i++) {
      fpigi = &fgi->players[i];
      row = mysql_fetch_row(res);
      if (row[0]) {
        fpigi->score = atof(row[0]);
      }
    }
    mysql_free_result(res);
    res = NULL;


    /* Fetch the players' rating changes for this game. */

    my_snprintf(buf, sizeof(buf),
        "SELECT r.old_rating, r.old_rating_deviation, "
        "    r.rating, r.rating_deviation "
        "  FROM players AS p LEFT JOIN ratings AS r"
        "    ON p.id = r.player_id"
        "  WHERE p.game_id = %d"
        "  ORDER BY p.rank",
        id);
    if (!fcdb_execute(sock, buf)) {
      goto ERROR;
    }

    res = mysql_store_result(sock);
    if (mysql_num_rows(res) != fgi->num_players) {
      goto ERROR;
    }

    for (i = 0; i < fgi->num_players; i++) {
      fpigi = &fgi->players[i];
      row = mysql_fetch_row(res);
      if (row[0]) {
        fpigi->old_rating = atof(row[0]);
      }
      if (row[1]) {
        fpigi->old_rd = atof(row[1]);
      }
      if (row[2]) {
        fpigi->new_rating = atof(row[2]);
      }
      if (row[3]) {
        fpigi->new_rd = atof(row[3]);
      }
    }
    mysql_free_result(res);
    res = NULL;

  } /* srvarg.fcdb.more_game_info == TRUE */


  /* Fetch information about the teams. */

  my_snprintf(buf, sizeof(buf),
      "SELECT name, result, rank, score FROM teams"
      "  WHERE game_id = %d"
      "  ORDER BY rank",
      id);
  if (!fcdb_execute(sock, buf)) {
    goto ERROR;
  }

  res = mysql_store_result(sock);
  fgi->num_teams = mysql_num_rows(res);
  if (fgi->num_teams > 0) {
    fgi->teams = fc_calloc(fgi->num_teams,
                           sizeof(struct fcdb_team_in_game_info));
    for (i = 0; i < fgi->num_teams; i++) {
      row = mysql_fetch_row(res);
      ftigi = &fgi->teams[i];
      sz_strlcpy(ftigi->name, row[0]);
      if (row[1]) {
        sz_strlcpy(ftigi->result, row[1]);
      }
      if (row[2]) {
        ftigi->rank = atof(row[2]);
      }
      if (row[3]) {
        ftigi->score = atof(row[3]);
      }
    }
  }
  mysql_free_result(res);
  res = NULL;

  fcdb_close(sock);
  return fgi;

ERROR:
  if (fgi) {
    fcdb_game_info_free(fgi);
  }
  if (res) {
    mysql_free_result(res);
  }
  return NULL;
#endif
  return NULL;
}

/**************************************************************************
  ...
**************************************************************************/
void fcdb_game_info_free(struct fcdb_game_info *fgi)
{
  if (!fgi) {
    return;
  }

  if (fgi->num_players > 0 && fgi->players != NULL) {
    free(fgi->players);
    fgi->num_players = 0;
    fgi->players = NULL;
  }
  if (fgi->num_teams > 0 && fgi->teams != NULL) {
    free(fgi->teams);
    fgi->num_teams = 0;
    fgi->teams = NULL;
  }
  free(fgi);
}

/**************************************************************************
  ...
**************************************************************************/
struct fcdb_topten_info *fcdb_topten_info_new(int type)
{
#ifdef HAVE_MYSQL
  MYSQL mysql, *sock = &mysql;
  char buf[1024];
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  struct fcdb_topten_info *ftti;
  struct fcdb_topten_info_entry *tte;
  int i;

  fcdb_connect_or_return(sock, NULL);

  my_snprintf(buf, sizeof(buf),
      "SELECT a.id, a.name, r.rating, r.rating_deviation"
      "  FROM %s AS a INNER JOIN ratings AS r INNER JOIN ("
      "    SELECT MAX(id) AS last_id FROM ratings"
      "      WHERE game_type = '%s'"
      "      GROUP BY user_id) AS rm"
      "    ON r.id = rm.last_id AND a.id = r.user_id"
      "  ORDER BY r.rating DESC"
      "  LIMIT 10",
      AUTH_TABLE, game_type_name_orig(type));
  fcdb_execute_or_return(sock, buf, NULL);

  res = mysql_store_result(sock);
  ftti = fc_calloc(1, sizeof(struct fcdb_topten_info));
  ftti->count = mysql_num_rows(res);

  if (ftti->count <= 0) {
    mysql_free_result(res);
    fcdb_close(sock);
    return ftti;
  }

  ftti->entries = fc_calloc(ftti->count,
                            sizeof(struct fcdb_topten_info_entry));

  for (i = 0; i < ftti->count; i++) {
    row = mysql_fetch_row(res);
    tte = &ftti->entries[i];
    tte->id = atoi(row[0]);
    sz_strlcpy(tte->user, row[1]);
    tte->rating = atof(row[2]);
    tte->rd = atof(row[3]);
  }

  mysql_free_result(res);

  for (i = 0; i < ftti->count; i++) {
    tte = &ftti->entries[i];
    my_snprintf(buf, sizeof(buf),
      "SELECT p.result, COUNT(p.result) "
      "  FROM ratings AS r INNER JOIN players AS p"
      "    ON r.player_id = p.id"
      "  WHERE r.user_id = %d AND r.game_type = '%s'"
      "  GROUP BY p.result"
      "  ORDER BY FIELD(p.result, 'win', 'lose', 'draw')"
      "  LIMIT 3",
      tte->id, game_type_name_orig(type));
    if (!fcdb_execute(sock, buf)) {
      goto ERROR;
    }
    res = mysql_store_result(sock);
    row = mysql_fetch_row(res);
    if (row != NULL && 0 == mystrcasecmp(row[0], "win")) {
      tte->wins = atoi(row[1]);
      row = mysql_fetch_row(res);
    }
    if (row != NULL && 0 == mystrcasecmp(row[0], "lose")) {
      tte->loses = atoi(row[1]);
      row = mysql_fetch_row(res);
    }
    if (row != NULL && 0 == mystrcasecmp(row[0], "draw")) {
      tte->draws = atoi(row[1]);
      row = mysql_fetch_row(res);
    }
    mysql_free_result(res);
    res = NULL;
  }

  fcdb_close(sock);
  return ftti;

ERROR:
  if (res) {
    mysql_free_result(res);
  }
  fcdb_topten_info_free(ftti);
  return NULL;

#else
  return NULL;
#endif /* HAVE_MYSQL */
}

/**************************************************************************
  ...
**************************************************************************/
void fcdb_topten_info_free(struct fcdb_topten_info *ftti)
{
  if (ftti == NULL) {
    return;
  }
  if (ftti->entries != NULL) {
    free(ftti->entries);
    ftti->entries = NULL;
  }
  free(ftti);
}

/**************************************************************************
  ...
**************************************************************************/
struct fcdb_gamelist *fcdb_gamelist_new(int type, const char *user,
                                        int first, int last)
{
#ifdef HAVE_MYSQL
  struct fcdb_gamelist *fgl = NULL;
  struct fcdb_gamelist_entry *fgle = NULL;
  MYSQL mysql, *sock = &mysql;
  char buf[1024], range_clause[128];
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  int i;
  const int MAX_GAMELIST_RESULTS = 50;

  fcdb_connect_or_return(sock, NULL);

  fgl = fc_calloc(1, sizeof(struct fcdb_gamelist));

  if (first > 0 || last > 0) {
    if (first < 1) {
      first = MAX(1, last - MAX_GAMELIST_RESULTS + 1);
    }
    if (last < first) {
      last = first + MAX_GAMELIST_RESULTS - 1;
    }
  }

  if (first > 0 && last > 0 && first <= last) {
    my_snprintf(range_clause, sizeof(range_clause),
                "AND g.id BETWEEN %d AND %d",
                first, last);
  } else {
    range_clause[0] = '\0';
  }

  if (user[0] != '\0') {
    if (!get_user_id(sock, user, &fgl->id)) {
      goto ERROR;
    }
    if (fgl->id <= 0) {
      return fgl;
    }
    my_snprintf(buf, sizeof(buf),
        "SELECT g.id, g.type, COUNT(*), UNIX_TIMESTAMP(g.created),"
        "    g.outcome"
        "  FROM games AS g INNER JOIN ratings AS r ON g.id = r.game_id"
        "    INNER JOIN players AS p ON g.id = p.game_id"
        "  WHERE r.user_id = %d %s"
        "  GROUP BY g.id"
        "  ORDER BY g.id DESC"
        "  LIMIT %d",
        fgl->id,
        range_clause,
        MAX_GAMELIST_RESULTS);
    if (!fcdb_execute(sock, buf)) {
      goto ERROR;
    }
  } else if (type != GT_NUM_TYPES) {
    my_snprintf(buf, sizeof(buf),
        "SELECT g.id, g.type, COUNT(*), UNIX_TIMESTAMP(g.created),"
        "    g.outcome"
        "  FROM games AS g INNER JOIN players AS p"
        "    ON g.id = p.game_id"
        "  WHERE g.type = '%s' %s"
        "  GROUP BY g.id"
        "  ORDER BY g.id DESC"
        "  LIMIT %d",
        game_type_name_orig(type),
        range_clause,
        MAX_GAMELIST_RESULTS);
    if (!fcdb_execute(sock, buf)) {
      goto ERROR;
    }
  } else {
    my_snprintf(buf, sizeof(buf),
        "SELECT g.id, g.type, COUNT(*), UNIX_TIMESTAMP(g.created),"
        "    g.outcome"
        "  FROM games AS g INNER JOIN players AS p"
        "    ON g.id = p.game_id"
        "  WHERE TRUE %s"
        "  GROUP BY g.id"
        "  ORDER BY g.id DESC"
        "  LIMIT %d",
        range_clause,
        MAX_GAMELIST_RESULTS);
    if (!fcdb_execute(sock, buf)) {
      goto ERROR;
    }
  }

  res = mysql_store_result(sock);
  fgl->count = mysql_num_rows(res);
  if (fgl->count > 0) {
    fgl->entries = fc_calloc(fgl->count,
                             sizeof(struct fcdb_gamelist_entry));
    for (i = fgl->count - 1; i >= 0; i--) {
      fgle = &fgl->entries[i];
      row = mysql_fetch_row(res);
      fgle->id = atoi(row[0]);
      sz_strlcpy(fgle->type, row[1]);
      fgle->players = atoi(row[2]);
      fgle->created = atol(row[3]);
      sz_strlcpy(fgle->outcome, row[4] ? row[4] : "<not set>");
    }
  }
  mysql_free_result(res);
  res = NULL;


  fcdb_close(sock);
  return fgl;

ERROR:
  if (fgl) {
    fcdb_gamelist_free(fgl);
  }
  if (res) {
    mysql_free_result(res);
  }
#endif /* HAVE_MYSQL */
  return NULL;
}

/**************************************************************************
  ...
**************************************************************************/
void fcdb_gamelist_free(struct fcdb_gamelist *fgl)
{
  if (fgl == NULL) {
    return;
  }
  if (fgl->entries) {
    free(fgl->entries);
    fgl->entries = NULL;
  }
  free(fgl);
}

/**************************************************************************
  ...
**************************************************************************/
struct fcdb_aliaslist *fcdb_aliaslist_new(const char *user)
{
#ifdef HAVE_MYSQL
  struct fcdb_aliaslist *fal = NULL;
  struct fcdb_aliaslist_entry *fale = NULL;
  MYSQL mysql, *sock = &mysql;
  char buf[1024];
  MYSQL_RES *res = NULL;
  MYSQL_ROW row;
  int i;
  char addr[256], caddr[256];

  fcdb_connect_or_return(sock, NULL);

  my_snprintf(buf, sizeof(buf),
    "SELECT id, address, createaddress FROM %s WHERE name = '%s'",
    AUTH_TABLE, fcdb_escape(sock, user));
  fcdb_reset_escape_buffer();
  fcdb_execute_or_return(sock, buf, NULL);

  fal = fc_calloc(1, sizeof(struct fcdb_aliaslist));

  res = mysql_store_result(sock);
  if (mysql_num_rows(res) != 1) {
    mysql_free_result(res);
    fcdb_close(sock);
    return fal;
  }

  row = mysql_fetch_row(res);
  fal->id = atoi(row[0]);
  sz_strlcpy(addr, row[1] ? row[1] : "");
  sz_strlcpy(caddr, row[2] ? row[2] : "");

  mysql_free_result(res);
  res = NULL;

  if (addr[0] != '\0' || caddr[0] != '\0') {
    if (addr[0] != '\0' && caddr[0] != '\0') {
      my_snprintf(buf, sizeof(buf),
        "SELECT id, name FROM %s"
        "  WHERE (address = '%s' OR address = '%s'"
        "    OR createaddress = '%s' OR createaddress = '%s')"
        "    AND id != %d",
        AUTH_TABLE, addr, caddr, addr, caddr, fal->id);
    } else if (addr[0] != '\0') {
      my_snprintf(buf, sizeof(buf),
        "SELECT id, name FROM %s"
        "  WHERE (address = '%s' OR createaddress = '%s')"
        "    AND id != %d",
        AUTH_TABLE, addr, addr, fal->id);
    } else {
      my_snprintf(buf, sizeof(buf),
        "SELECT id, name FROM %s"
        "  WHERE (address = '%s' OR createaddress = '%s')"
        "    AND id != %d",
        AUTH_TABLE, caddr, caddr, fal->id);
    }
    if (!fcdb_execute(sock, buf)) {
      fcdb_aliaslist_free(fal);
      return NULL;
    }
    res = mysql_store_result(sock);
    fal->count = mysql_num_rows(res);

    if (fal->count > 0) {
      fal->entries = fc_calloc(fal->count,
                               sizeof(struct fcdb_aliaslist_entry));
      for (i = 0; i < fal->count; i++) {
        row = mysql_fetch_row(res);
        fale = &fal->entries[i];
        fale->id = atoi(row[0]);
        if (row[1]) {
          sz_strlcpy(fale->name, row[1]);
        }
      }
    }
    mysql_free_result(res);
    res = NULL;
  }

  fcdb_close(sock);

  return fal;

#endif /* HAVE_MYSQL */
  return NULL;
}
/**************************************************************************
  ...
**************************************************************************/
void fcdb_aliaslist_free(struct fcdb_aliaslist *fal)
{
  if (fal == NULL) {
    return;
  }
  if (fal->entries) {
    free(fal->entries);
    fal->entries = NULL;
  }
  free(fal);
}

/**************************************************************************
  Returns TRUE and sets srvarg.auth.salted if the auth database supports
  salted passwords. Otherwise, returns FALSE and a warning message is
  printed in the server console.
**************************************************************************/
bool fcdb_check_salted_passwords(void)
{
#ifdef HAVE_MYSQL
  MYSQL mysql, *sock = &mysql;
  MYSQL_RES *res;
  unsigned count;

  fcdb_connect_or_return(sock, FALSE);

  res = mysql_list_fields(sock, AUTH_TABLE, "salt");
  count = mysql_num_fields(res);
  mysql_free_result(res);

  if (count != 1) {
    freelog(LOG_ERROR,
        "\n******************* Database Warning ******************\n\n"
        "The auth table '%s' in database '%s' does not\n"
        "support salted passwords. Using salt would improve the\n"
        "security of the users' passwords. The script authsalt in\n"
        "the contrib diretory can be used to modify your current\n"
        "auth table to support password salt.\n",
        AUTH_TABLE, fcdb.dbname);
    srvarg.auth.salted = FALSE;
    return FALSE;
  }

  srvarg.auth.salted = TRUE;
  return TRUE;
#endif /* HAVE_MYSQL */

  srvarg.auth.salted = FALSE;
  return FALSE;
}
