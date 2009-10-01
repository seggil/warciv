/********************************************************************** 
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32_NATIVE
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "capability.h"
#include "capstr.h"
#include "dataio.h"
#include "fcintl.h"
#include "game.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "packets.h"
#include "registry.h"
#include "support.h"
#include "version.h"

#include "agents.h"
#include "attribute.h"
#include "chatline_g.h"
#include "civclient.h"
#include "climisc.h"
#include "connectdlg_common.h"
#include "connectdlg_g.h"
#include "dialogs_g.h"		/* popdown_races_dialog() */
#include "gui_main_g.h"		/* add_net_input(), remove_net_input() */
#include "menu_g.h"
#include "messagewin_g.h"
#include "options.h"
#include "packhand.h"
#include "pages_g.h"
#include "plrdlg_g.h"
#include "repodlgs_g.h"

#include "clinet.h"

struct connection aconnection;
static int socklan;
static struct server_list *lan_servers;
static union my_sockaddr server_addr;
bool server_has_extglobalinfo = FALSE;

#define ASYNC_SLIST_CTX_MEMORY_GUARD 0xadebac1e

/* used by create_server_list_async */
struct async_slist_ctx {
  int guard;
  server_list_created_callback_t callback;
  void *userdata;
  data_free_func_t datafree;
  char errbuf[128];
  int sock;
  bool have_data_to_write;
  int input_id;
  int req_id;
  int nlsa_id;
  int buflen;			/* amount of data in buf */
  char buf[65536];		/* metaserver response buffer */
  char urlpath[MAX_LEN_ADDR];
  char metaname[MAX_LEN_ADDR];
  int metaport;
};

/* used by cancel_async_server_list_request */
static struct hash_table *async_slist_req_table = NULL;
static int async_slist_req_id = 0;

/**************************************************************************
  ...
**************************************************************************/
static void check_init_async_tables(void)
{
  if (!async_slist_req_table) {
    async_slist_req_table = hash_new(hash_fval_int, hash_fcmp_int);
  }
}

/*************************************************************************
  Close socket and cleanup.  This one doesn't print a message, so should
  do so before-hand if necessary.
**************************************************************************/
static void close_socket_nomessage(struct connection *pc)
{
  if (save_options_on_exit) {
    save_options();
  }
  connection_common_close(pc);
  remove_net_input();
  popdown_races_dialog(); 
  close_connection_dialog();

  /* We need to clear the voteinfo when a client disconnects.
   * Unfortunately doing it only in client_game_free is not
   * sufficient. */
  voteinfo_queue_init();

  set_client_page(PAGE_MAIN);

  reports_force_thaw();

  set_client_state(CLIENT_PRE_GAME_STATE);
  agents_disconnect();
  update_menus();
  client_remove_all_cli_conn();
  aconnection.player = NULL;
}

/**************************************************************************
...
**************************************************************************/
static void client_close_socket_callback(struct connection *pc)
{
  close_socket_nomessage(pc);
  /* If we lost connection to the internal server - kill him */
  client_kill_server(TRUE);
  append_network_statusbar(_("Lost connection to server!"));
  freelog(LOG_NORMAL, "lost connection to server");
}

/**************************************************************************
  Connect to a civserver instance -- or at least try to.  On success,
  return 0; on failure, put an error message in ERRBUF and return -1.
**************************************************************************/
int connect_to_server(const char *username, const char *hostname, int port,
		      char *errbuf, int errbufsize)
{
  if (get_server_address(hostname, port, errbuf, errbufsize) != 0) {
    return -1;
  }

  if (try_to_connect(username, errbuf, errbufsize) != 0) {
    return -1;
  }
  return 0;
}

/**************************************************************************
  Get ready to [try to] connect to a server:
   - translate HOSTNAME and PORT (with defaults of "localhost" and
     DEFAULT_SOCK_PORT respectively) to a raw IP address and port number, and
     store them in the `server_addr' variable
   - return 0 on success
     or put an error message in ERRBUF and return -1 on failure
**************************************************************************/
int get_server_address(const char *hostname, int port, char *errbuf,
		       int errbufsize)
{
  if (port == 0)
    port = DEFAULT_SOCK_PORT;

  /* use name to find TCP/IP address of server */
  if (!hostname)
    hostname = "localhost";

  if (!net_lookup_service(hostname, port, &server_addr)) {
    mystrlcpy(errbuf, _("Failed looking up host."), errbufsize);
    return -1;
  }

  return 0;
}

/**************************************************************************
  Try to connect to a server (get_server_address() must be called first!):
   - try to create a TCP socket and connect it to `server_addr'
   - if successful:
	  - start monitoring the socket for packets from the server
	  - send a "login request" packet to the server
      and - return 0
   - if unable to create the connection, close the socket, put an error
     message in ERRBUF and return the Unix error code (ie., errno, which
     will be non-zero).
**************************************************************************/
int try_to_connect(const char *username, char *errbuf, int errbufsize)
{
  struct packet_server_join_req req;

  close_socket_set_callback(client_close_socket_callback);

  /* connection in progress? wait. */
  if (aconnection.used) {
    mystrlcpy(errbuf, _("Connection in progress."), errbufsize);
    return -1;
  }
  
  if ((aconnection.sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    mystrlcpy(errbuf, mystrsocketerror(mysocketerrno()),
                     errbufsize);
    return -1;
  }

  if (connect(aconnection.sock, &server_addr.sockaddr,
      sizeof(server_addr)) == -1) {
    mystrlcpy(errbuf, mystrsocketerror(mysocketerrno()), errbufsize);
    my_closesocket(aconnection.sock);
    aconnection.sock = -1;
    return myerrno();
  }

  connection_common_init(&aconnection);
  aconnection.client.last_request_id_used = 0;
  aconnection.client.last_processed_request_id_seen = 0;
  aconnection.client.request_id_of_currently_handled_packet = 0;
  aconnection.incoming_packet_notify = notify_about_incoming_packet;
  aconnection.outgoing_packet_notify = notify_about_outgoing_packet;

  /* call gui-dependent stuff in gui_main.c */
  add_net_input(aconnection.sock);

  /* now send join_request package */

  req.major_version = MAJOR_VERSION;
  req.minor_version = MINOR_VERSION;
  req.patch_version = PATCH_VERSION;
  sz_strlcpy(req.version_label, VERSION_LABEL);
  sz_strlcpy(req.capability, our_capability);
  sz_strlcpy(req.username, username);
  
  send_packet_server_join_req(&aconnection, &req);

  return 0;
}

/**************************************************************************
...
**************************************************************************/
void disconnect_from_server(void)
{
  const bool force = !aconnection.used;

  attribute_flush();
  /* If it's internal server - kill him 
   * We assume that we are always connected to the internal server  */
  if (!force) {
    client_kill_server(FALSE);
  }
  close_socket_nomessage(&aconnection);
  if (force) {
    client_kill_server(TRUE);
  }
  append_network_statusbar(_("Disconnected from server."));
}  

/**************************************************************************
A wrapper around read_socket_data() which also handles the case the
socket becomes writeable and there is still data which should be sent
to the server.

Returns:
    -1  :  an error occurred - you should close the socket
    >0  :  number of bytes read
    =0  :  no data read, would block
**************************************************************************/
static int read_from_connection(struct connection *pc, bool block)
{
  freelog(LOG_DEBUG, "client read_from_connection");

  for (;;) {
    fd_set readfs, writefs, exceptfs;
    bool have_data_for_server = (pc->used && pc->send_buffer
				&& pc->send_buffer->ndata > 0);
    int n;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    MY_FD_ZERO(&readfs);
    FD_SET(pc->sock, &readfs);

    MY_FD_ZERO(&exceptfs);
    FD_SET(pc->sock, &exceptfs);

    if (have_data_for_server) {
      MY_FD_ZERO(&writefs);
      FD_SET(pc->sock, &writefs);
      n = select(pc->sock + 1, &readfs, &writefs, &exceptfs,
		 block ? NULL : &tv);
    } else {
      n = select(pc->sock + 1, &readfs, NULL, &exceptfs,
		 block ? NULL : &tv);
    }

    /* the socket is neither readable, writeable nor got an
       exception */
    if (n == 0) {
      return 0;
    }

    if (n == -1) {
      long err_no = mysocketerrno();

      if (is_interrupted_errno(err_no)) {
	/* EINTR can happen sometimes, especially when compiling with -pg.
	 * Generally we just want to run select again. */
	freelog(LOG_DEBUG, "select() returned EINTR");
	continue;
      }

      freelog(LOG_NORMAL, "error in select() return=%d errno=%lu (%s)",
	      n, err_no, mystrsocketerror(err_no));
      return -1;
    }

    if (FD_ISSET(pc->sock, &exceptfs)) {
      return -1;
    }

    if (have_data_for_server && FD_ISSET(pc->sock, &writefs)) {
      flush_connection_send_buffer_all(pc);
    }

    if (FD_ISSET(pc->sock, &readfs)) {
      int rv;
      
      rv = read_socket_data(pc, pc->buffer);

      return rv < 0 ? -1 : rv;
    }
  }
}

/**************************************************************************
 This function is called when the client received a new input from the
 server.
**************************************************************************/
void input_from_server(int fd)
{
  assert(fd == aconnection.sock);

  freelog(LOG_DEBUG, "input_from_server");

  if (read_from_connection(&aconnection, FALSE) >= 0) {
    enum packet_type type;

    while (TRUE) {
      bool result;
      void *packet = get_packet_from_connection(&aconnection,
						&type, &result);

      if (result) {
	assert(packet != NULL);
	handle_packet_input(packet, type);
	free(packet);
      } else {
	assert(packet == NULL);
	break;
      }
    }
  } else {
    client_close_socket_callback(&aconnection);
  }
}

/**************************************************************************
 This function will sniff at the given fd, get the packet and call
 handle_packet_input. It will return if there is a network error or if
 the PACKET_PROCESSING_FINISHED packet for the given request is
 received.
**************************************************************************/
void input_from_server_till_request_got_processed(int fd, 
						  int expected_request_id)
{
  assert(expected_request_id);
  assert(fd == aconnection.sock);

  freelog(LOG_DEBUG,
	  "input_from_server_till_request_got_processed("
	  "expected_request_id=%d)", expected_request_id);

  while (TRUE) {
    if (read_from_connection(&aconnection, TRUE) >= 0) {
      enum packet_type type;

      while (TRUE) {
	bool result;
	void *packet = get_packet_from_connection(&aconnection,
						  &type, &result);
	if (!result) {
	  assert(packet == NULL);
	  break;
	}

	assert(packet != NULL);
	handle_packet_input(packet, type);
	free(packet);

	if (type == PACKET_PROCESSING_FINISHED) {
	  freelog(LOG_DEBUG, "ifstrgp: expect=%d, seen=%d",
		  expected_request_id,
		  aconnection.client.last_processed_request_id_seen);
	  if (aconnection.client.last_processed_request_id_seen >=
	      expected_request_id) {
	    freelog(LOG_DEBUG, "ifstrgp: got it; returning");
	    return;
	  }
	}
      }
    } else {
      client_close_socket_callback(&aconnection);
      break;
    }
  }
}

#ifdef WIN32_NATIVE
/*****************************************************************
   Returns an uname like string for windows
*****************************************************************/
static char *win_uname()
{
  static char uname_buf[256];
  char cpuname[16];
  char *osname;
  SYSTEM_INFO sysinfo;
  OSVERSIONINFO osvi;

  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osvi);

  switch (osvi.dwPlatformId) {
  case VER_PLATFORM_WIN32s:
    osname = "Win32s";
    break;

  case VER_PLATFORM_WIN32_WINDOWS:
    osname = "Win32";

    if (osvi.dwMajorVersion == 4) {
      switch (osvi.dwMinorVersion) {
      case 0:
	osname = "Win95";
	break;
      case 10:
	osname = "Win98";
	break;
      case 90:
	osname = "WinME";
	break;
      default:
	break;
      }
    }
    break;

  case VER_PLATFORM_WIN32_NT:
    osname = "WinNT";

    if (osvi.dwMajorVersion == 5) {
      switch (osvi.dwMinorVersion) {
      case 0:
	osname = "Win2000";
	break;
      case 1:
	osname = "WinXP";
	break;
      case 2:
	osname = "WinServer2003";
	break;
      default:
	break;
      }
    }

    if (osvi.dwMajorVersion == 6) {
      switch (osvi.dwMinorVersion) {
      case 0:
	osname = "WinVista";
	break;
      default:
	break;
      }
    }
    break;

  default:
    osname = osvi.szCSDVersion;
    break;
  }

  GetSystemInfo(&sysinfo); 
  switch (sysinfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL:
      {
	unsigned int ptype;
	if (sysinfo.wProcessorLevel < 3) /* Shouldn't happen. */
	  ptype = 3;
	else if (sysinfo.wProcessorLevel > 9) /* P4 */
	  ptype = 6;
	else
	  ptype = sysinfo.wProcessorLevel;
	
	my_snprintf(cpuname, sizeof(cpuname), "i%d86", ptype);
      }
      break;

    case PROCESSOR_ARCHITECTURE_MIPS:
      sz_strlcpy(cpuname, "mips");
      break;

    case PROCESSOR_ARCHITECTURE_ALPHA:
      sz_strlcpy(cpuname, "alpha");
      break;

    case PROCESSOR_ARCHITECTURE_PPC:
      sz_strlcpy(cpuname, "ppc");
      break;

    case PROCESSOR_ARCHITECTURE_AMD64:
      sz_strlcpy(cpuname, "amd64");
      break;
    case PROCESSOR_ARCHITECTURE_IA64:
      sz_strlcpy(cpuname, "ia64");
      break;

    case PROCESSOR_ARCHITECTURE_UNKNOWN:
    default:
      sz_strlcpy(cpuname, "unknown");
      break;
  }
  my_snprintf(uname_buf, sizeof(uname_buf),
	      "%s %ld.%ld [%s]", osname, osvi.dwMajorVersion,
	      osvi.dwMinorVersion, cpuname);
  return uname_buf;
}
#endif

/**************************************************************************
 The server sends a stream in a registry 'ini' type format.
 Read it using secfile functions and fill the server_list structs.
**************************************************************************/
static struct server_list *parse_metaserver_data(fz_FILE *f)
{
  struct server_list *server_list;
  struct section_file the_file, *file = &the_file;
  int nservers, i, j;

  server_list = server_list_new();

  /* This call closes f. */
  if (!section_file_load_from_stream(file, f)) {
    return server_list;
  }

  nservers = secfile_lookup_int_default(file, 0, "main.nservers");

  for (i = 0; i < nservers; i++) {
    char *host, *port, *version, *state, *message, *nplayers, *patches;
    struct server *pserver =
	(struct server *) fc_malloc(sizeof(struct server));

    host = secfile_lookup_str_default(file, "", "server%d.host", i);
    pserver->host = mystrdup(host);

    port = secfile_lookup_str_default(file, "", "server%d.port", i);
    pserver->port = mystrdup(port);

    version = secfile_lookup_str_default(file, "", "server%d.version", i);
    pserver->version = mystrdup(version);

    state = secfile_lookup_str_default(file, "", "server%d.state", i);
    pserver->state = mystrdup(state);

    message = secfile_lookup_str_default(file, "", "server%d.message", i);
    pserver->message = mystrdup(message);

    patches = secfile_lookup_str_default(file, "none", "server%d.patches", i);
    pserver->patches = mystrdup(patches);

    nplayers = secfile_lookup_str_default(file, "0", "server%d.nplayers", i);
    pserver->nplayers = atoi(nplayers);

    if (pserver->nplayers > 0) {
      pserver->players =
	fc_malloc(pserver->nplayers * sizeof(*pserver->players));
    } else {
      pserver->players = NULL;
    }
      
    for (j = 0; j < pserver->nplayers; j++) {
      char *name, *user, *nation, *type, *host;

      name = secfile_lookup_str_default(file, "", 
                                        "server%d.player%d.name", i, j);
      pserver->players[j].name = mystrdup(name);

      user = secfile_lookup_str_default(file, "",
					"server%d.player%d.user", i, j);
      pserver->players[j].user = mystrdup(user);

      type = secfile_lookup_str_default(file, "",
                                        "server%d.player%d.type", i, j);
      pserver->players[j].type = mystrdup(type);

      host = secfile_lookup_str_default(file, "", 
                                        "server%d.player%d.host", i, j);
      pserver->players[j].host = mystrdup(host);

      nation = secfile_lookup_str_default(file, "",
                                          "server%d.player%d.nation", i, j);
      pserver->players[j].nation = mystrdup(nation);
    }

    for (j = 0; section_file_lookup(file, "server%d.vars%d.name", i, j); j++);

    pserver->nvars = j;
    if (pserver->nvars > 0) {
      pserver->vars = fc_calloc(pserver->nvars, sizeof(*pserver->vars));
    } else {
      pserver->vars = NULL;
    }

    for (j = 0; j < pserver->nvars; j++) {
      char *vars;
      vars = secfile_lookup_str_default(file, "",
                                        "server%d.vars%d.name", i, j);
      pserver->vars[j].name = mystrdup(vars);
      vars = secfile_lookup_str_default(file, "",
                                        "server%d.vars%d.value", i, j);
      pserver->vars[j].value = mystrdup(vars);
    }
    server_list_append(server_list, pserver);
  }

  section_file_free(file);
  return server_list;
}

/**************************************************************************
  Fills in buf with the http request that will be sent to the metaserver.
  Returns the number of bytes of buf used.
**************************************************************************/
static int generate_server_list_http_request(char *buf, int buflen,
                                             const char *urlpath,
                                             const char *metaname,
                                             int metaport)
{
  char machine_string[128];
  const char *capstr;
  int len;
#ifdef HAVE_UNAME
  struct utsname un;
#endif 

#ifdef HAVE_UNAME
  uname(&un);
  my_snprintf(machine_string,sizeof(machine_string),
	      "%s %s [%s]", un.sysname, un.release, un.machine);
#else /* ! HAVE_UNAME */
  /* Fill in here if you are making a binary without sys/utsname.h and know
     the OS name, release number, and machine architechture */
#ifdef WIN32_NATIVE
  sz_strlcpy(machine_string,win_uname());
#else
  my_snprintf(machine_string,sizeof(machine_string),
              "unknown unknown [unknown]");
#endif
#endif /* HAVE_UNAME */

  capstr = my_url_encode(our_capability);

  len = my_snprintf(buf, buflen,
    "POST %s HTTP/1.1\r\n"
    "Host: %s:%d\r\n"
    "User-Agent: Freeciv/%s %s %s\r\n"
    "Connection: close\r\n"
    "Content-Type: application/x-www-form-urlencoded; charset=\"utf-8\"\r\n"
    "Content-Length: %lu\r\n"
    "\r\n"
    "client_cap=%s\r\n",
    urlpath,
    metaname, metaport,
    VERSION_STRING, client_string, machine_string,
    (unsigned long) (strlen("client_cap=") + strlen(capstr)),
    capstr);
  return len;
}

/**************************************************************************
  ...
**************************************************************************/
static struct server_list *parse_metaserver_http_data(fz_FILE * f,
						      char *errbuf,
						      int n_errbuf)
{
  char str[1024];

  if (f == NULL) {
    /* TRANS: This means a network error when trying to connect to
     * the metaserver.  The message will be shown directly to the user. */
    mystrlcpy(errbuf, _("Failed querying socket"), n_errbuf);
    return NULL;
  }

  /* skip HTTP headers */
  while (fz_fgets(str, sizeof(str), f) && strcmp(str, "\r\n") != 0) {
    /* nothing */
  }

  /* XXX: TODO check for magic Content-Type: text/x-ini -vasc */

  /* parse HTTP message body */
  return parse_metaserver_data(f);
}

/**************************************************************************
  ...
**************************************************************************/
static void destroy_async_slist_ctx(void *data)
{
  struct async_slist_ctx *ctx = data;

  freelog(LOG_DEBUG, "dasc destroy_async_slist_ctx data=%p", ctx);

  if (!ctx) {
    return;
  }

  if (ctx->guard != ASYNC_SLIST_CTX_MEMORY_GUARD) {
    freelog(LOG_DEBUG, "dasc called more than once on %p", ctx);
    return;
  }
  ctx->guard = 0;

  if (ctx->nlsa_id > 0) {
    freelog(LOG_DEBUG, "dasc cancelling net_lookup id=%d", ctx->nlsa_id);
    cancel_net_lookup_service(ctx->nlsa_id);
    ctx->nlsa_id = -1;
  }

  if (ctx->input_id > 0) {
    freelog(LOG_DEBUG, "dasc removing net input id%d", ctx->input_id);
    remove_net_input_callback(ctx->input_id);
    ctx->input_id = -1;
  }

  if (ctx->sock != -1) {
    freelog(LOG_DEBUG, "dasc closing socket %d", ctx->sock);
    my_closesocket(ctx->sock);
    ctx->sock = -1;
  }
  
  if (ctx->req_id > 0) {
    freelog(LOG_DEBUG, "dasc deleting ctx=%p req_id=%d from aslr table",
            ctx, ctx->req_id);
    hash_delete_entry(async_slist_req_table,
                      INT_TO_PTR(ctx->req_id));
    ctx->req_id = -1;
  }

  freelog(LOG_DEBUG, "dasc free async_slist_ctx %p", ctx);
  free(ctx);
}

/**************************************************************************
  ...
**************************************************************************/
static void async_slist_error(struct async_slist_ctx *ctx, char *fmt, ...)
{
  va_list ap;
  char errbuf[256];

  freelog(LOG_DEBUG, "ase async_slist_error ctx=%p", ctx);

  assert(ctx != NULL);

  va_start(ap, fmt);
  my_vsnprintf(errbuf, sizeof(errbuf), fmt, ap);
  va_end(ap);

  freelog(LOG_DEBUG, "ase error=\"%s\" ctx->callback=%p", errbuf, ctx->callback);

  if (ctx->callback) {
    freelog(LOG_DEBUG, "ase calling %p userdata=%p", ctx->callback,
            ctx->userdata);
    (*ctx->callback) (NULL, errbuf, ctx->userdata);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void process_metaserver_response(struct async_slist_ctx *ctx)
{
  FILE *fp;
  struct server_list *sl;
  fz_FILE *f;
  char errbuf[256];
  char *newbuf;
  int i, j, newsize = 0;

  freelog(LOG_DEBUG, "pmr process_metaserver_response ctx=%p", ctx);

  if (!(fp = my_tmpfile())) {
    async_slist_error(ctx, _("Could not create temporary file: %s"),
                      mystrerror(myerrno()));
    return;
  }

  newbuf = fc_malloc(sizeof(ctx->buf));
  newsize = ctx->buflen;

  /* Prevent \" sequences in files which would produce problem for secfile_lookup 
     Insert space character between \ and " */
  for (i = 0, j = 0; i < ctx->buflen; i++, j++) {
    if (i > 0 && ctx->buf[i-1] == '\\' && ctx->buf[i] == '"' ) {
      newbuf[j++]= ' ';
      newsize++;
    }
    newbuf[j] = ctx->buf[i];
  }

  if (newsize != fwrite(newbuf, 1, newsize, fp)) {
    async_slist_error(ctx, _("Error writing to temporary file: %s"),
                      mystrerror(myerrno()));
    fclose(fp);
    free(newbuf);
    return;
  }
  free(newbuf);
  fflush(fp);
  rewind(fp);

  /* fp assumed to be closed by parse_metaserver_data */
  f = fz_from_stream(fp);
  sl = parse_metaserver_http_data(f, errbuf, sizeof(errbuf));
  if (ctx->callback) {
    freelog(LOG_DEBUG, "pmr calling %p userdata=%p", ctx->callback, ctx->userdata);
    (*ctx->callback) (sl, errbuf, ctx->userdata);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static bool metaserver_read_cb(int sock, int flags, void *data)
{
  char *buf;
  int nb = 0, count = 0, rem = 0;
  static const int READSZ = 4096;
  struct async_slist_ctx *ctx = data;
  long err_no = 0;

  assert(data != NULL);
  assert(ctx->sock == sock);

  freelog(LOG_DEBUG, "mrc metaserver_read_cb ctx=%p flags=%d", ctx, flags);

  if (flags & INPUT_ERROR) {
    async_slist_error(ctx, _("Error while waiting for "
                             "metaserver response."));
    return FALSE;		/* remove input callback */
  }

  if ((flags & INPUT_CLOSED) && !(flags & INPUT_READ)) {
    if (ctx->buflen > 0) {	/* we received some data before, it might be all */
      goto FINISHED_READING_DATA;
    } else {			/* this is definitely an error */
      async_slist_error(ctx, _("Metaserver closed the connection "
                               "while we were waiting for a response."));
    }
    return FALSE;		/* remove input callback */
  }

  freelog(LOG_DEBUG, "mrc reading response...");

  buf = ctx->buf + ctx->buflen;
  rem = sizeof(ctx->buf) - ctx->buflen - 1;

  while (rem > 0) {
    nb = my_readsocket(ctx->sock, buf, rem > READSZ ? READSZ : rem);
    err_no = mysocketerrno();
    freelog(LOG_DEBUG, "mrc my_readsocket nb=%d", nb);
    if (nb <= 0) {
      break;
    }
    rem -= nb;
    buf += nb;
    count += nb;
#ifndef WIN32_NATIVE
/*  g-io-channel-win32-new-socket said */
/*  Polling a GSource created to watch a channel for a socket puts the socket  */
/*  in non-blocking mode.  */
/*  This is a side-effect of the implementation and unavoidable. */
/*  It appears that we must keep reading sockets until  */
/*  error occurs or finish reading (nb == 0) otherwise G_IO_HUP GCond */
/*  nearly never occurs */
   if (nb < READSZ) {
      break;
    }
#endif
  }
  ctx->buflen += count;
  freelog(LOG_DEBUG, "mrc count=%d (buflen=%d)", count, ctx->buflen);

  if (nb == -1) {		/* read error */
    if (my_socket_would_block(err_no)) {
      freelog(LOG_DEBUG, "mrc socket read would block");
      return TRUE;
    }
    async_slist_error(ctx, _("Read from socket failed: %s."),
                      mystrsocketerror(mysocketerrno()));
    return FALSE;
  }

  if (rem == 0) {		/* buffer capacity exceeded */
    async_slist_error(ctx, _("Metaserver response exceeds "
                             "buffer capacity."));
    return FALSE;
  }

  if (count == 0 || nb == 0 || flags & INPUT_CLOSED) {
    /* connection closed */
    freelog(LOG_DEBUG, "mrc connection closed, we are done reading");

FINISHED_READING_DATA:
    ctx->buf[ctx->buflen] = '\0';
    process_metaserver_response(ctx);
    destroy_async_slist_ctx(ctx);
    return FALSE;		/* we are done reading */
  }

  freelog(LOG_DEBUG, "mrc waiting for more data...");
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool metaserver_write_cb(int sock, int flags, void *data)
{
  int nb = 0;
  struct async_slist_ctx *ctx = data;
  long err_no = 0;

  assert(ctx != NULL);
  assert(ctx->sock == sock);

  freelog(LOG_DEBUG, "mwc metaserver_write_cb ctx=%p flags=%d", ctx, flags);

  if (flags & INPUT_ERROR) {
    async_slist_error(ctx, _("Failed to connect to metaserver."));
    return FALSE;              /* remove input callback */
  }

  if (flags & INPUT_CLOSED) {
    async_slist_error(ctx, _("Metaserver closed the connection "
                             "before it was even established!"));
    return FALSE;              /* remove input callback */
  }

  freelog(LOG_DEBUG, "mwc connection to metaserver succeeded");

  while (ctx->buflen > 0) {
    nb = my_writesocket(ctx->sock, ctx->buf, ctx->buflen);
    err_no = mysocketerrno();
    freelog(LOG_DEBUG, "mwc my_writesocket nb=%d sock=%d msg=\"%s\"",
            nb, ctx->sock, mystrsocketerror(err_no));
    if (nb <= 0) {
      break;
    }

    assert(ctx->buflen >= nb);
    ctx->buflen -= nb;
    if (nb > 0 && ctx->buflen > 0) {
      memmove(ctx->buf, ctx->buf + nb, ctx->buflen);
    }
  }

  if (nb < 0) {
    if (my_socket_would_block(err_no)) {
      freelog(LOG_DEBUG, "mwc socket write would block");
      return TRUE;
    }
    async_slist_error(ctx,_("Write error during send of metaserver "
                            "request: %s (%d)"),
                      mystrsocketerror(mysocketerrno()),
                      mysocketerrno());
    return FALSE;
  }

  if (nb == 0) {
    freelog(LOG_DEBUG, "mwc my_writesocket wrote 0 bytes :(");
    return TRUE;
  }

  ctx->have_data_to_write = FALSE;
  ctx->input_id = add_net_input_callback(ctx->sock,
      INPUT_READ | INPUT_ERROR | INPUT_CLOSED,
      metaserver_read_cb, ctx, NULL);

  freelog(LOG_DEBUG, "mwc added read callback");
  return FALSE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool metaserver_name_lookup_callback(union my_sockaddr *addr_result,
                                            void *data)
{
  int sock, res, len;
  struct async_slist_ctx *ctx = data;
  struct sockaddr *addr;
  long err_no;

  freelog(LOG_DEBUG, "mnlc metaserver_name_lookup_callback: "
          "addr_result=%p data=%p", addr_result, data);

  assert(ctx != NULL);

  ctx->nlsa_id = -1;

  if (!addr_result) {
    async_slist_error(ctx, _("Failed looking up host"));
    return TRUE;
  }

  addr = &addr_result->sockaddr;

  if (-1 == (sock = socket(AF_INET, SOCK_STREAM, 0))) {
    async_slist_error(ctx, _("Socket call failed: %s"),
                      mystrsocketerror(mysocketerrno()));
    return TRUE;
  }
  ctx->sock = sock;

#ifndef WIN32_NATIVE
/*   if we have winsock, we must let's glib to make this  */
/*   socket in  non-blocking mode to prevent busy wait socket*/
  if (-1 == my_set_nonblock(sock)) {
    async_slist_error(ctx, _("Could not set non-blocking mode: %s"),
                      mystrsocketerror(mysocketerrno()));
    return TRUE;
  }
#endif

  res = connect(sock, addr, sizeof(struct sockaddr));
  err_no = mysocketerrno();

  if (my_socket_operation_in_progess(err_no)
      || my_socket_would_block(err_no)) {
    res = 0;
  }

  if (res == -1) {
    async_slist_error(ctx, _("Connect operation failed: %s"),
                      mystrsocketerror(err_no));
    return TRUE;
  }

  ctx->sock = sock;
  ctx->have_data_to_write = TRUE;
  len = generate_server_list_http_request(ctx->buf, sizeof(ctx->buf),
                                          ctx->urlpath, ctx->metaname,
                                          ctx->metaport);
  ctx->buflen = len;

  freelog(LOG_DEBUG, "mnlc %d byte generated request copied to "
          "write buffer", len);

  ctx->input_id = add_net_input_callback(sock,
      INPUT_WRITE | INPUT_CLOSED | INPUT_ERROR,
      metaserver_write_cb, ctx, NULL);

  freelog(LOG_DEBUG, "mnlc waiting for socket %d to become writable", sock);

  return TRUE;
}

/**************************************************************************
  Cancels an asynchronous server list request in progress. id is the
  return value from create_server_list_async.
**************************************************************************/
void *cancel_async_server_list_request(int id)
{
  struct async_slist_ctx *ctx;
  void *ret = NULL;

  freelog(LOG_DEBUG, "caslr cancel_async_server_list_request id=%d", id);

  assert(id > 0);

  check_init_async_tables();

  freelog(LOG_DEBUG, "caslr deleting id=%d from aslr table", id);
  ctx = hash_delete_entry(async_slist_req_table, INT_TO_PTR(id));

  if (!ctx) {
    freelog(LOG_DEBUG, "caslr id=%d not in table!", id);
    return NULL;
  }

  freelog(LOG_DEBUG, "caslr id=%d found in table ctx=%p", id, ctx);
  if (ctx->datafree) {
    freelog(LOG_DEBUG, "caslr calling datafree %p on %p", ctx->datafree,
      ctx->userdata);
    (*ctx->datafree) (ctx->userdata);
  } else {
    ret = ctx->userdata;
  }
  freelog(LOG_DEBUG, "caslr destroying async_slist_ctx %p", ctx);
  destroy_async_slist_ctx(ctx);
  return ret;
}

/**************************************************************************
 Create the list of servers from the metaserver asynchronously.
 Arguments:
   errbuf - used for the description of an error, if it occurs.
   n_errbuf - maximum size of the buffer.
   cb - function to call once the list is obtained.
   data - to be passed to cb.
   datafree - called on data if this request is cancelled with
     cancel_async_server_list_request.
 Returns:
   -1: an error occured, cb and datafree are not called, and errbuf is
      filled in.
    0: cb is called directly (i.e. it has already been called once this
      function returns). datafree is not called on data, and errbuf is
      untouched.
    >0: an id that can be passed to cancel_async_server_list_request. The
      request is now in progress and cb will be called when it is done.
**************************************************************************/
int create_server_list_async(char *errbuf, int n_errbuf,
                             server_list_created_callback_t cb,
                             void *data, data_free_func_t datafree)
{
  const char *urlpath;
  char metaname[MAX_LEN_ADDR];
  int metaport, nlsa_id;
  struct async_slist_ctx *ctx;

  freelog(LOG_DEBUG, "csla create_server_list_async cb=%p data=%p "
          "datafree=%p", cb, data, datafree);

  check_init_async_tables();

  assert(n_errbuf > 0);
  assert(errbuf != NULL);
  errbuf[0] = '\0';

  urlpath = my_lookup_httpd(metaname, &metaport, default_metaserver);
  if (!urlpath) {
    my_snprintf(errbuf, n_errbuf,
          _("Invalid $http_proxy or metaserver value (\"%s\");"
            " must start with \"http://\"."),
          default_metaserver);
    return -1;
  }

  ctx = fc_calloc(1, sizeof(struct async_slist_ctx));
  freelog(LOG_DEBUG, "csla new async_slist_ctx %p", ctx);
  ctx->guard = ASYNC_SLIST_CTX_MEMORY_GUARD;

  mystrlcpy(ctx->urlpath, urlpath, strlen(urlpath) + 1);
  mystrlcpy(ctx->metaname, metaname, strlen(metaname) + 1);
  ctx->metaport = metaport;
  ctx->callback = cb;
  ctx->userdata = data;
  ctx->datafree = datafree;
  ctx->sock = -1;
  ctx->input_id = -1;
  ctx->req_id = -1;
  ctx->nlsa_id = -1;

  if (disable_custom_dns) {
    union my_sockaddr addr;
    if (!net_lookup_service(metaname, metaport, &addr)) {
      my_snprintf(errbuf, n_errbuf,
                  _("Failed looking up metaserver hostname "
                    "%s:%d (using system DNS)"),
                  metaname, metaport);
      destroy_async_slist_ctx(ctx);
      return -1;
    } else {
      metaserver_name_lookup_callback(&addr, ctx);
      nlsa_id = 0;
    }
  } else {
    nlsa_id = net_lookup_service_async(metaname, metaport,
                                       metaserver_name_lookup_callback,
                                       ctx, NULL);
  }
  freelog(LOG_DEBUG, "csla got nlsa_id = %d", nlsa_id);

  if (nlsa_id == -1) {
    my_snprintf(errbuf, n_errbuf, 
        _("Could not initiate an asynchronous name lookup "
          "for metaserver %s:%d"), metaname, metaport);
    freelog(LOG_DEBUG, "csla destroying ctx %p", ctx);
    destroy_async_slist_ctx(ctx);
    return -1;
  }

  if (nlsa_id == 0 && ctx->input_id <= 0) {
    freelog(LOG_DEBUG, "csla destroying ctx %p", ctx);
    destroy_async_slist_ctx(ctx);
    return 0;
  }

  assert(nlsa_id > 0 || ctx->input_id > 0);

  ctx->nlsa_id = nlsa_id;
  ctx->req_id = ++async_slist_req_id;
  freelog(LOG_DEBUG, "csla req_id = %d", ctx->req_id);

#ifdef DEBUG
  freelog(LOG_DEBUG, "csla about to insert key %d into table...",
          ctx->req_id);
  freelog(LOG_DEBUG, "csla table entries=%d buckets=%d deleted=%d",
          hash_num_entries(async_slist_req_table),
          hash_num_buckets(async_slist_req_table),
          hash_num_deleted(async_slist_req_table));
#endif

  hash_insert(async_slist_req_table, INT_TO_PTR(ctx->req_id), ctx);

  freelog(LOG_DEBUG, "csla inserted ctx=%p req_id=%d into aslr table",
          ctx, ctx->req_id);

  /* continued in metaserver_name_lookup_callback */

  return ctx->req_id;
}

/**************************************************************************
 Create the list of servers from the metaserver
 The result must be free'd with delete_server_list() when no
 longer used
**************************************************************************/
struct server_list *create_server_list(char *errbuf, int n_errbuf)
{
  union my_sockaddr addr;
  int s;
  fz_FILE *f;
  const char *urlpath;
  char metaname[MAX_LEN_ADDR];
  int metaport;
  char str[MAX_LEN_PACKET];

  urlpath = my_lookup_httpd(metaname, &metaport, default_metaserver);
  if (!urlpath) {
    mystrlcpy(errbuf, _("Invalid $http_proxy or metaserver value, must "
                        "start with 'http://'"),
              n_errbuf);
    return NULL;
  }

  if (!net_lookup_service(metaname, metaport, &addr)) {
    mystrlcpy(errbuf, _("Failed looking up host"), n_errbuf);
    return NULL;
  }

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    mystrlcpy(errbuf, mystrsocketerror(mysocketerrno()), n_errbuf);
    return NULL;
  }

  if (connect(s, (struct sockaddr *) &addr.sockaddr, sizeof(addr)) == -1) {
    mystrlcpy(errbuf, mystrsocketerror(mysocketerrno()), n_errbuf);
    my_closesocket(s);
    return NULL;
  }

  generate_server_list_http_request(str, sizeof(str), urlpath,
				    metaname, metaport);
  f = my_querysocket(s, str, strlen(str));

  return parse_metaserver_http_data(f, errbuf, n_errbuf);
}

/**************************************************************************
 Frees everything associated with a server list including
 the server list itself (so the server_list is no longer
 valid after calling this function)
**************************************************************************/
void delete_server_list(struct server_list *server_list)
{
  server_list_iterate(server_list, ptmp) {
    int i;

    free(ptmp->host);
    free(ptmp->port);
    free(ptmp->version);
    free(ptmp->state);
    free(ptmp->message);
    free(ptmp->patches);

    if (ptmp->players) {
      for (i = 0; i < ptmp->nplayers; i++) {
        free(ptmp->players[i].name);
        free(ptmp->players[i].user);
        free(ptmp->players[i].type);
        free(ptmp->players[i].host);
        free(ptmp->players[i].nation);
      }
      free(ptmp->players);
    }

    if (ptmp->vars) {
      for (i = 0; i < ptmp->nvars; i++) {
        free(ptmp->vars[i].name);
        free(ptmp->vars[i].value);
      }
      free(ptmp->vars);
    }

    free(ptmp);
  } server_list_iterate_end;

  server_list_unlink_all(server_list);
  free(server_list);
}

/**************************************************************************
  Broadcast an UDP package to all servers on LAN, requesting information 
  about the server. The packet is send to all Freeciv servers in the same
  multicast group as the client.
**************************************************************************/
int begin_lanserver_scan(void)
{
  union my_sockaddr addr;
  struct data_out dout;
  int sock, opt = 1;
  unsigned char buffer[MAX_LEN_PACKET];
  struct ip_mreq mreq;
  const char *group;
  unsigned char ttl;
  size_t size;

  /* Create a socket for broadcasting to servers. */
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    freelog(LOG_ERROR, "socket failed: %s",
            mystrsocketerror(mysocketerrno()));
    return 0;
  }

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                 (char *)&opt, sizeof(opt)) == -1) {
    freelog(LOG_ERROR, "SO_REUSEADDR failed: %s",
            mystrsocketerror(mysocketerrno()));
  }

  /* Set the UDP Multicast group IP address. */
  group = get_multicast_group();
  memset(&addr, 0, sizeof(addr));
  addr.sockaddr_in.sin_family = AF_INET;
  addr.sockaddr_in.sin_addr.s_addr = inet_addr(get_multicast_group());
  addr.sockaddr_in.sin_port = htons(SERVER_LAN_PORT);

  /* Set the Time-to-Live field for the packet  */
  ttl = SERVER_LAN_TTL;
  if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&ttl, 
                 sizeof(ttl))) {
    freelog(LOG_ERROR, "setsockopt failed: %s",
            mystrsocketerror(mysocketerrno()));
    return 0;
  }

  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, 
                 sizeof(opt))) {
    freelog(LOG_ERROR, "setsockopt failed: %s",
            mystrsocketerror(mysocketerrno()));
    return 0;
  }

  dio_output_init(&dout, buffer, sizeof(buffer));
  dio_put_uint8(&dout, SERVER_LAN_VERSION);
  size = dio_output_used(&dout);
 

  if (sendto(sock, buffer, size, 0, &addr.sockaddr, sizeof(addr)) < 0) {
    freelog(LOG_ERROR, "sendto failed: %s",
            mystrsocketerror(mysocketerrno()));
    return 0;
  } else {
    freelog(LOG_DEBUG, ("Sending request for server announcement on LAN."));
  }

  my_closesocket(sock);

  /* Create a socket for listening for server packets. */
  if ((socklan = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    freelog(LOG_ERROR, "socket failed: %s",
            mystrsocketerror(mysocketerrno()));
    return 0;
  }

  my_set_nonblock(socklan);

  if (setsockopt(socklan, SOL_SOCKET, SO_REUSEADDR,
                 (char *)&opt, sizeof(opt)) == -1) {
    freelog(LOG_ERROR, "SO_REUSEADDR failed: %s",
            mystrsocketerror(mysocketerrno()));
  }
                                                                               
  memset(&addr, 0, sizeof(addr));
  addr.sockaddr_in.sin_family = AF_INET;
  addr.sockaddr_in.sin_addr.s_addr = htonl(INADDR_ANY); 
  addr.sockaddr_in.sin_port = htons(SERVER_LAN_PORT + 1);

  if (bind(socklan, &addr.sockaddr, sizeof(addr)) < 0) {
    freelog(LOG_ERROR, "bind failed: %s",
            mystrsocketerror(mysocketerrno()));
    return 0;
  }

  mreq.imr_multiaddr.s_addr = inet_addr(group);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(socklan, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                 (const char*)&mreq, sizeof(mreq)) < 0) {
    freelog(LOG_ERROR, "setsockopt failed: %s",
            mystrsocketerror(mysocketerrno()));
    return 0;
  }

  lan_servers = server_list_new();

  return 1;
}

/**************************************************************************
  Listens for UDP packets broadcasted from a server that responded
  to the request-packet sent from the client. 
**************************************************************************/
struct server_list *get_lan_server_list(void)
{
#ifdef HAVE_SOCKLEN_T
  socklen_t fromlen;
#else
  int fromlen;
#endif
  union my_sockaddr fromend;
  struct hostent *from;
  char msgbuf[128];
  int type;
  struct data_in din;
  char servername[512];
  char port[256];
  char version[256];
  char status[256];
  char players[256];
  char message[1024];
  fd_set readfs, exceptfs;
  struct timeval tv;

 again:

  FD_ZERO(&readfs);
  FD_ZERO(&exceptfs);
  FD_SET(socklan, &exceptfs);
  FD_SET(socklan, &readfs);
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  while (select(socklan + 1, &readfs, NULL, &exceptfs, &tv) == -1) {
    long err_no = mysocketerrno();

    if (is_interrupted_errno(err_no)) {
      /* EINTR can happen sometimes, especially when compiling with -pg.
       * Generally we just want to run select again. */
      continue;
    }
    freelog(LOG_ERROR, "select failed: %s",
            mystrsocketerror(err_no));
    return lan_servers;
  }

  if (!FD_ISSET(socklan, &readfs)) {
    return lan_servers;
  }

  dio_input_init(&din, msgbuf, sizeof(msgbuf));
  fromlen = sizeof(fromend);

  /* Try to receive a packet from a server. */ 
  if (0 < recvfrom(socklan, msgbuf, sizeof(msgbuf), 0,
                   &fromend.sockaddr, &fromlen)) {
    struct server *pserver;

    dio_get_uint8(&din, &type);
    if (type != SERVER_LAN_VERSION) {
      goto again;
    }
    dio_get_string(&din, servername, sizeof(servername));
    dio_get_string(&din, port, sizeof(port));
    dio_get_string(&din, version, sizeof(version));
    dio_get_string(&din, status, sizeof(status));
    dio_get_string(&din, players, sizeof(players));
    dio_get_string(&din, message, sizeof(message));

    if (!mystrcasecmp("none", servername)) {
      from = gethostbyaddr((char *) &fromend.sockaddr_in.sin_addr,
			   sizeof(fromend.sockaddr_in.sin_addr), AF_INET);
      sz_strlcpy(servername, inet_ntoa(fromend.sockaddr_in.sin_addr));
    }

    /* UDP can send duplicate or delayed packets. */
    server_list_iterate(lan_servers, aserver) {
      if (!mystrcasecmp(aserver->host, servername) 
          && !mystrcasecmp(aserver->port, port)) {
	goto again;
      } 
    } server_list_iterate_end;

    freelog(LOG_DEBUG,
            ("Received a valid announcement from a server on the LAN."));
    
    pserver =  (struct server*)fc_malloc(sizeof(struct server));
    pserver->host = mystrdup(servername);
    pserver->port = mystrdup(port);
    pserver->version = mystrdup(version);
    pserver->patches = mystrdup("Cannot be read");
    pserver->state = mystrdup(status);
    pserver->nplayers = atoi(players);
    pserver->message = mystrdup(message);
    pserver->players = NULL;
    pserver->nvars = 0;
    pserver->vars = NULL;

    server_list_prepend(lan_servers, pserver);

    goto again;
  }

  return lan_servers;
}

/**************************************************************************
  Closes the socket listening on the lan and frees the list of LAN servers.
**************************************************************************/
void finish_lanserver_scan(void) 
{
  my_closesocket(socklan);
  delete_server_list(lan_servers);
}

/**************************************************************************
  Returns TRUE if the client is connected to a warserver >= 0.8.9a.
**************************************************************************/
bool is_warserver(void)
{
  return (aconnection.established
	  && has_capability("extroutes", aconnection.capability));
}
