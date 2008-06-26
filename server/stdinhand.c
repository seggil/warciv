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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_LIBREADLINE
#include <readline/readline.h>
#ifdef HAVE_NEWLIBREADLINE
#define completion_matches(x,y) rl_completion_matches(x,y)
#define filename_completion_function rl_filename_completion_function
#endif
#endif
#include "fciconv.h"
#include "astring.h"
#include "capability.h"
#include "capstr.h"
#include "events.h"
#include "fcintl.h"
#include "game.h"
#include "hash.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "packets.h"
#include "player.h"
#include "rand.h"
#include "registry.h"
#include "shared.h" /* fc__attribute, bool type, etc. */
#include "support.h"
#include "timing.h"
#include "version.h"

#include "citytools.h"
#include "commands.h"
#include "connecthand.h"
#include "console.h"
#include "database.h"
#include "diplhand.h"
#include "gamehand.h"
#include "gamelog.h"
#include "handchat.h"
#include "mapgen.h"
#include "maphand.h"
#include "meta.h"
#include "plrhand.h"
#include "report.h"
#include "ruleset.h"
#include "sanitycheck.h"
#include "savegame.h"
#include "score.h"
#include "sernet.h"
#include "settings.h"
#include "srv_main.h"
#include "vote.h"

#include "advmilitary.h" /* assess_danger_player() */
#include "ailog.h"

#include "stdinhand.h"
/* Import */
#include "stdinhand_info.h"

static bool cut_client_connection(struct connection *caller, char *name,
                                  bool check);
static bool show_help(struct connection *caller, char *arg);
static bool show_list(struct connection *caller, char *arg);
static void show_connections(struct connection *caller);
static void show_actionlist(struct connection *caller);
static void show_teams(struct connection *caller, bool send_to_all);
static void show_rulesets(struct connection *caller);
static bool show_scenarios(struct connection *caller);
static bool show_mutes(struct connection *caller);
static bool set_ai_level(struct connection *caller, char *name, int level,
                         bool check);
static bool set_away(struct connection *caller, char *name, bool check);
static bool is_allowed_to_take(struct player *pplayer, bool will_obs,
                               char *msg);
static bool observe_command(struct connection *caller, char *name,
                            bool check);
static bool take_command(struct connection *caller, char *name, bool check);
static bool detach_command(struct connection *caller, char *name,
                           bool check);
static bool start_command(struct connection *caller, char *name,
                          bool check);
static bool end_command(struct connection *caller, char *str, bool check);
static bool ban_command(struct connection *caller, char *pattern,
                        bool check);
static bool unban_command(struct connection *caller, char *pattern,
                          bool check);
static bool addaction_command(struct connection *caller, char *pattern,
                              bool check);
static bool delaction_command(struct connection *caller, char *pattern,
                              bool check);
static bool reset_command(struct connection *caller, bool check);

#define ACTION_LIST_FILE_VERSION 1
static int load_action_list_v0(const char *filename);
static int load_action_list_v1(const char *filename);
static int load_action_list(const char *filename);
static int save_action_list(const char *filename);
bool loadactionlist_command(struct connection *caller,
                            char *filename, bool check);
bool saveactionlist_command(struct connection *caller,
                            char *filename, bool check);
bool clearactionlist_command(struct connection *caller,
                             char *filename, bool check);
static bool loadscenario_command(struct connection *caller, char *str,
                                 bool check);

/* Team names contributed by book, Pendragon, hima,
 * Not Logged In, Zachron and many more from the
 * multiplayer freeciv community. Thanks people! */
static char *two_team_suggestions[][2] = {
  {"ALPHA", "BETA"},
  {"GOOD", "EVIL"},
  {"CAPULETS", "MONTAGUES"},
  {"BLACK", "WHITE"},
  {"GREY", "GREYER"},
  {"FAT", "SLIM"},
  {"NAUGHTY", "NICE"},
  {"COPS", "ROBBERS"},
  {"ZERG", "PROTOSS"},
  {"ORCS", "HUMANS"},
  {"LILLIPUT", "BLEFUSCU"},
  {"HALF_FULL", "HALF_EMPTY"},
  {"SHEEP", "WOLVES"},
  {"BIG_ENDIAN", "LITTLE_ENDIAN"},
  {"FANATICS", "HERETICS"},
  {"TITANS", "OLYMPIANS"},
  {"BOURGEOISIE", "PROLETARIAT"},
  {"VOTERS", "POLITCIANS"},
  {"OPTIMATES", "POPULARES"},
  {"BLOODS", "CRIPS"},
  {"TRIADS", "YAKUZA"},
  {"SCIENTOLOGISTS", "AUM_SHINRIKYO"},
  {"PEOPLES_TEMPLE", "HEAVENS_GATE"},
  {"DARK", "LIGHT"},
  {"YOUNG", "OLD"},
  {"KITTENS", "PUPPIES"},
  {"FRUITS", "VEGETABLES"},
  {"POINTERS", "REFERENCES"},
  {"CLASSES", "STRUCTS"},
  {"INTS", "FLOATS"},
  {"LEGS", "BREASTS"},
  {"BOYS", "GIRLS"},
  {"PLUS", "MINUS"},
  {"POSITIVE", "NEGATIVE"},
  {"YIN", "YANG"},
  {"CATS", "DOGS"},
  {"ARM", "CORE"},
  {"NOD", "GDI"},
  {"FELLOWSHIP", "MORDOR"},
  {"GALACTIC_EMPIRE", "REBEL_ALLIANCE"},
  {"AXIS", "ALLIES"},
  {"NATO", "WARZAW_PACT"},
  {"DEMOCRATS", "REPUBLICANS"},
  {"TORIES", "LABOUR"},
  {"CIA", "KGB"},
  {"MACS", "PCS"},
  {"LEFT", "RIGHT"},
  {"NORTH", "SOUTH"},
  {"EAST", "WEST"},
  {"ANGELS", "DEMONS"},
  {"ATREIDES", "HARKONNEN"},
  {"INDULGENCE", "ABSTINENCE"},
  {"PIMPS", "DEALERS"},
  {"APPLES", "ORANGES"},
  {"FOO", "BAR"},
  {"CATHODES", "ANODES"},
  {"SEEDERS", "LEECHERS"},
  {"FIRE", "WATER"},
  {"ELVES", "TROLLS"},
  {"DRAGONS", "KNIGHTS"},
  {"BEAUTY", "TRUTH"},
  {"POLAR", "RECTANGULAR"},
  {"SINES", "COSINES"},
  {"DOCTORS", "PATIENTS"},
  {"EUKARYOTES", "PROKARYOTES"},
  {"COBOL", "FORTRAN"},
  {"FERMIONS", "BOSONS"},
  {"NINJAS", "PIRATES"},
  {"WEYL", "RICCI"},
  {"HARD", "SOFT"},
  {"ANARCHISTS", "COMMUNISTS"},
  {"LOYALISTS", "FASCISTS"},
  {"MODS", "ROCKERS"},
  {"GUNS", "ROSES"},
  {"TOP", "BOTTOM"},
  {"UP", "DOWN"},
  {"BEER", "LAGER"},
  {"ORANGE", "BLUE"},
  {"BLUNT", "SHARP"},
  {"TRUTH", "BEAUTY"},
  {"SWORDS", "PLOUGHSHARES"},
  {"CHICKENS", "EGGS"},
  {"HIPPIES", "YUPPIES"},
  {"HALLOWEEN", "CHRISTMAS"},
  {"FINAL_FANTASY", "DRAGON_QUEST"},
  {"TREKKIES", "JEDIS"},
  {"SALT", "PEPPER"},
  {"EAST_COAST", "WEST_COAST"},
  {"PEPSI", "COKE"},
  {"BOOKS", "MOVIES"},
  {"NARNIA", "MIDDLE_EARTH"},
  {"THE_SIMPSONS", "SOUTHPARK"},
  {"LADIES", "GENTLEMEN"},
  {"SMALLPOX", "LARGEPOX"},
  {"DIGITAL", "ANALOGUE"},
  {"PACIFISTS", "MILITARISTS"},
  {"GREENPEACE", "CORPORATIONS"},
  {"CHESS", "CHECKERS"},
  {"CRUSADERS", "PAGANS"},
  {"JIHADISTS", "INFIDELS"},
  {"HEGEMONY", "OUSTERS"},
  {"VORLONS", "SHADOWS"},
  {"IMPERIALISTS", "SEPARATISTS"},
  {"THEISTS", "ATHEISTS"},
  {"POLYTHEISTS", "MONOTHEISTS"},
  {"WINDOWS", "LINUX"},
};
static char *three_team_suggestions[][3] = {
  {"HERBIVORES", "CARNIVORES", "OMNIVORES"},
  {"RED", "GREEN", "BLUE"},
  {"ALPHA", "BETA", "GAMMA"},
  {"YES", "NO", "MAYBE"},
  {"ABORT", "RETRY", "FAIL"},
  {"READY", "SET", "GO"},
  {"BREAKFAST", "LUNCH", "DINNER"},
  {"SUBJECT", "OBJECT", "VERB"},
  {"ROCK", "PAPER", "SCISSORS"},
  {"LEFT", "RIGHT", "CENTER"},
  {"ENDOMORPHS", "MESOMORPHS", "ECTOMORPHS"},
  {"ID", "EGO", "SUPEREGO"},
  {"REPTILES", "MAMMALS", "INSECTS"},
  {"DEMOCRATS", "REPUBLICANS", "INDEPENDANTS"},
  {"TERRAN", "ZERG", "PROTOSS"},
  {"HALLOWEEN", "CHRISTMAS", "THANKSGIVING"},
  {"KETCHUP", "MUSTARD", "MAYONAISE"},
  {"CHOCOLATE", "VANILLA", "STRAWBERRY"},
  {"STREET_FIGHTER", "MORTAL_KOMBAT", "DEAD_OR_ALIVE"},
};

static char *four_team_suggestions[][4] = {
  {"ATREIDES", "HARKONNEN", "ORDOS", "CORRINO"},
  {"NORTH", "SOUTH", "WEST", "EAST"},
  {"CYAN", "MAGENTA", "YELLOW", "BLACK"},
  {"UP", "DOWN", "LEFT", "RIGHT"},
  {"ELECTROMAGNETISM", "GRAVITY", "STRONG", "WEAK"},
  {"EARTH", "WIND", "FIRE", "WATER"},
  {"GUANINE", "CYTOSINE", "THYMINE", "ADENINE"},
  {"ORCS", "HUMANS", "UNDEAD", "NIGHT_ELVES"},
  {"PEACHES", "PLUMS", "APRICOTS", "POMAGRANATES"},
  {"COTTON", "WOOL", "SILK", "POLYESTER"},
  {"GRYFFINDOR", "HUFFLEPUFF", "RAVENCLAW", "SLITHERYN"},
  {"VINYL", "8TRACK", "CASSETTE", "CD"},
};

struct muteinfo {
  int conn_id;
  int turns_left;
  char *addr;
};

static struct hash_table *mute_table;

struct kickinfo {
  time_t time_of_kick;
  char addr[64];
};

static struct hash_table *kick_table;

static const char horiz_line[] =
    "------------------------------------------------------------------------------";

typedef enum {
  PNameOk,
  PNameEmpty,
  PNameTooLong,
  PNameIllegal
} PlayerNameStatus;

/**************************************************************************
...
**************************************************************************/
static PlayerNameStatus test_player_name(char *name)
{
  size_t len = strlen(name);
  if (len == 0) {
    return PNameEmpty;
  } else if (len > MAX_LEN_NAME - 1) {
    return PNameTooLong;
  } else if (mystrcasecmp(name, ANON_PLAYER_NAME) == 0) {
    return PNameIllegal;
  } else if (mystrcasecmp(name, OBSERVER_NAME) == 0) {
    return PNameIllegal;
  }
  return PNameOk;
}

static const char *cmdname_accessor(int i)
{
  return commands[i].name;
}

/**************************************************************************
  Convert a named command into an id.
  If accept_ambiguity is true, return the first command in the
  enum list which matches, else return CMD_AMBIGOUS on ambiguity.
  (This is a trick to allow ambiguity to be handled in a flexible way
  without importing notify_player() messages inside this routine - rp)
**************************************************************************/
static enum command_id command_named(const char *token,
                                     bool accept_ambiguity)
{
  enum m_pre_result result;
  int ind;
  result = match_prefix(cmdname_accessor, CMD_NUM, 0,
                        mystrncasecmp, token, &ind);
  if (result < M_PRE_AMBIGUOUS) {
    return ind;
  } else if (result == M_PRE_AMBIGUOUS) {
    return accept_ambiguity ? ind : CMD_AMBIGUOUS;
  } else {
    return CMD_UNRECOGNIZED;
  }
}

/**************************************************************************
  Initialize stuff related to this code module.
**************************************************************************/
void stdinhand_init(void)
{
  if (!mute_table) {
    mute_table = hash_new(hash_fval_string2, hash_fcmp_string);
  }

  if (!kick_table) {
    kick_table = hash_new(hash_fval_string2, hash_fcmp_string);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void unmute_conn_by_mi(struct muteinfo *mi)
{
  struct connection *pconn;

  if (mi == NULL) {
    return;
  }

  pconn = find_conn_by_id(mi->conn_id);

  hash_delete_entry(mute_table, mi->addr);
  free(mi->addr);
  free(mi);

  if (!pconn) {
    return;
  }
  notify_conn(pconn->self, _("Server: You have been unmuted."));
}

/**************************************************************************
  Update stuff every turn that is related to this code module. Run this
  on turn end.
**************************************************************************/
void stdinhand_turn(void)
{
  hash_iterate(mute_table, void *, key, struct muteinfo *, mi) {
    if (mi->turns_left > 0) {
      mi->turns_left--;
      if (mi->turns_left == 0) {
        unmute_conn_by_mi(mi);
      }
    }
  } hash_iterate_end;
}

/**************************************************************************
  Deinitialize stuff related to this code module.
**************************************************************************/
void stdinhand_free(void)
{
  if (mute_table) {
    hash_iterate(mute_table, void *, key, struct muteinfo *, mi) {
      free(mi->addr);
      free(mi);
    } hash_iterate_end;
    hash_free(mute_table);
    mute_table = NULL;
  }

  if (kick_table) {
    hash_iterate(kick_table, void *, key, struct kickinfo *, ki) {
      free(ki);
    } hash_iterate_end;
    hash_free(kick_table);
    kick_table = NULL;
  }
}

/**************************************************************************
  Whether the caller can use the specified command. caller == NULL means 
  console.
**************************************************************************/
static bool may_use(struct connection *caller, enum command_id cmd)
{
  if (!caller) {
    return command_access_level(cmd) < ALLOW_NEVER;
  }
  return caller->access_level >= command_access_level(cmd);
}

/**************************************************************************
  Whether the caller cannot use any commands at all.
  caller == NULL means console.
**************************************************************************/
static bool may_use_nothing(struct connection *caller)
{
  if (!caller) {
    return FALSE; /* on the console, everything is allowed */
  }
  return (caller->access_level == ALLOW_NONE);
}

/**************************************************************************
  Whether the caller can set the specified option (assuming that
  the state of the game would allow changing the option at all).
  caller == NULL means console.
**************************************************************************/
static bool may_set_option(struct connection *caller, int option_idx)
{
  if (!caller) {
    return TRUE; /* on the console, everything is allowed */
  } else {
    int level = caller->access_level;
    return ((level == ALLOW_HACK)
            || (level >= sset_access_level(option_idx)
                && sset_is_to_client(option_idx)));
  }
}

/**************************************************************************
  Whether the caller can set the specified option, taking into account
  access, and the game state.  caller == NULL means console.
**************************************************************************/
static bool may_set_option_now(struct connection *caller, int option_idx)
{
  return (may_set_option(caller, option_idx)
          && sset_is_changeable(option_idx));
}

/**************************************************************************
  Whether the caller can SEE the specified option.
  caller == NULL means console, which can see all.
  client players can see "to client" options, or if player
  has command access level to change option.
**************************************************************************/
static bool may_view_option(struct connection *caller, int option_idx)
{
  if (!caller) {
    return TRUE; /* on the console, everything is allowed */
  } else {
    return sset_is_to_client(option_idx)
        || may_set_option(caller, option_idx);
  }
}

/**************************************************************************
  feedback related to server commands
  caller == NULL means console.
  No longer duplicate all output to console.

  This lowlevel function takes a single line; prefix is prepended to line.
**************************************************************************/
static void cmd_reply_line(enum command_id cmd,
                           struct connection *caller,
                           enum rfc_status rfc_status,
                           const char *prefix, const char *line)
{
  const char *cmdname = cmd < CMD_NUM
      ? commands[cmd].name : cmd == CMD_AMBIGUOUS ? _("(ambiguous)")
      : cmd == CMD_UNRECOGNIZED ? _("(unknown)")
      : "(?!?)";                /* this case is a bug! */

  if (caller) {
    notify_conn(caller->self, "/%s: %s%s", cmdname, prefix, line);
    /* cc: to the console - testing has proved it's too verbose - rp
     * con_write(rfc_status, "%s/%s: %s%s", caller->name, cmdname, prefix, line);
     */
  } else {
    con_write(rfc_status, "%s%s", prefix, line);
  }
  if (rfc_status == C_OK) {
    conn_list_iterate(game.est_connections, pconn) {
      /* Do not tell caller, since he was told above! */
      if (pconn != caller) {
        notify_conn(pconn->self, _("Server: %s"), line);
      }
    } conn_list_iterate_end;
  }
}

/**************************************************************************
  va_list version which allow embedded newlines, and each line is sent
  separately. 'prefix' is prepended to every line _after_ the first line.
**************************************************************************/
static void vcmd_reply_prefix(enum command_id cmd,
                              struct connection *caller,
                              enum rfc_status rfc_status,
                              const char *prefix, const char *format,
                              va_list ap)
{
  char buf[4096];
  char *c0, *c1;
  my_vsnprintf(buf, sizeof(buf), format, ap);
  c0 = buf;
  while ((c1 = strstr(c0, "\n"))) {
    *c1 = '\0';
    cmd_reply_line(cmd, caller, rfc_status, (c0 == buf ? "" : prefix), c0);
    c0 = c1 + 1;
  }
  cmd_reply_line(cmd, caller, rfc_status, (c0 == buf ? "" : prefix), c0);
}

/**************************************************************************
  var-args version of above
  duplicate declaration required for attribute to work...
**************************************************************************/
static void cmd_reply_prefix(enum command_id cmd, struct connection *caller,
                             enum rfc_status rfc_status, const char *prefix,
                             const char *format, ...)
fc__attribute((__format__(__printf__, 5, 6)));
static void cmd_reply_prefix(enum command_id cmd, struct connection *caller,
                             enum rfc_status rfc_status, const char *prefix,
                             const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vcmd_reply_prefix(cmd, caller, rfc_status, prefix, format, ap);
  va_end(ap);
}

/**************************************************************************
  var-args version as above, no prefix
**************************************************************************/
static void cmd_reply(enum command_id cmd, struct connection *caller,
                      enum rfc_status rfc_status, const char *format, ...)
fc__attribute((__format__(__printf__, 4, 5)));
static void cmd_reply(enum command_id cmd, struct connection *caller,
                      enum rfc_status rfc_status, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vcmd_reply_prefix(cmd, caller, rfc_status, "", format, ap);
  va_end(ap);
}

/**************************************************************************
...
**************************************************************************/
static void cmd_reply_no_such_player(enum command_id cmd,
                                     struct connection *caller,
                                     char *name,
                                     enum m_pre_result match_result)
{
  switch (match_result) {
  case M_PRE_EMPTY:
    cmd_reply(cmd, caller, C_SYNTAX,
              _("Name is empty, so cannot be a player."));
    break;
  case M_PRE_LONG:
    cmd_reply(cmd, caller, C_SYNTAX,
              _("Name is too long, so cannot be a player."));
    break;
  case M_PRE_AMBIGUOUS:
    cmd_reply(cmd, caller, C_FAIL,
              _("Player name prefix '%s' is ambiguous."), name);
    break;
  case M_PRE_FAIL:
    cmd_reply(cmd, caller, C_FAIL,
              _("No player by the name of '%s'."), name);
    break;
  default:
    cmd_reply(cmd, caller, C_FAIL,
              _("Unexpected match_result %d (%s) for '%s'."),
              match_result, _(m_pre_description(match_result)), name);
    freelog(LOG_ERROR,
            "Unexpected match_result %d (%s) for '%s'.",
            match_result, m_pre_description(match_result), name);
  }
}

/**************************************************************************
...
**************************************************************************/
static void cmd_reply_no_such_conn(enum command_id cmd,
                                   struct connection *caller,
                                   const char *name,
                                   enum m_pre_result match_result)
{
  switch (match_result) {
  case M_PRE_EMPTY:
    cmd_reply(cmd, caller, C_SYNTAX,
              _("Name is empty, so cannot be a connection."));
    break;
  case M_PRE_LONG:
    cmd_reply(cmd, caller, C_SYNTAX,
              _("Name is too long, so cannot be a connection."));
    break;
  case M_PRE_AMBIGUOUS:
    cmd_reply(cmd, caller, C_FAIL,
              _("Connection name prefix '%s' is ambiguous."), name);
    break;
  case M_PRE_FAIL:
    cmd_reply(cmd, caller, C_FAIL,
              _("No connection by the name of '%s'."), name);
    break;
  default:
    cmd_reply(cmd, caller, C_FAIL,
              _("Unexpected match_result %d (%s) for '%s'."),
              match_result, _(m_pre_description(match_result)), name);
    freelog(LOG_ERROR,
            "Unexpected match_result %d (%s) for '%s'.",
            match_result, m_pre_description(match_result), name);
  }
}

/**************************************************************************
...
**************************************************************************/
static void open_metaserver_connection(struct connection *caller)
{
  server_open_meta();
  if (send_server_info_to_metaserver(META_INFO)) {
    notify_conn(NULL, _("Open metaserver connection to [%s]."),
                meta_addr_port());
  }
}

/**************************************************************************
...
**************************************************************************/
static void close_metaserver_connection(struct connection *caller)
{
  if (send_server_info_to_metaserver(META_GOODBYE)) {
    server_close_meta();
    notify_conn(NULL, _("Close metaserver connection to [%s]."),
                meta_addr_port());
  }
}

/**************************************************************************
...
**************************************************************************/
static bool metaconnection_command(struct connection *caller, char *arg,
                                   bool check)
{
  if ((*arg == '\0') || (0 == strcmp(arg, "?"))) {
    if (is_metaserver_open()) {
      cmd_reply(CMD_METACONN, caller, C_COMMENT,
                _("Metaserver connection is open."));
    } else {
      cmd_reply(CMD_METACONN, caller, C_COMMENT,
                _("Metaserver connection is closed."));
    }
  } else if ((0 == mystrcasecmp(arg, "u"))
             || (0 == mystrcasecmp(arg, "up"))) {
    if (!is_metaserver_open()) {
      if (!check) {
        open_metaserver_connection(caller);
      }
    } else {
      cmd_reply(CMD_METACONN, caller, C_METAERROR,
                _("Metaserver connection is already open."));
      return FALSE;
    }
  } else if ((0 == mystrcasecmp(arg, "d")) ||
             (0 == mystrcasecmp(arg, "down"))) {
    if (is_metaserver_open()) {
      if (!check) {
        close_metaserver_connection(caller);
      }
    } else {
      cmd_reply(CMD_METACONN, caller, C_METAERROR,
                _("Metaserver connection is already closed."));
      return FALSE;
    }
  } else {
    cmd_reply(CMD_METACONN, caller, C_METAERROR,
              _("Argument must be 'u', 'up', 'd', 'down', or '?'."));
    return FALSE;
  }
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static bool welcome_message_command(struct connection *caller,
                                    char *arg, bool check)
{
  if (check) {
    return TRUE;
  }

  if (0 == strcmp(arg, "default")) {
    if (welcome_message) {
      free(welcome_message);
      welcome_message = NULL;
    }
    cmd_reply(CMD_WELCOME_MESSAGE, caller, C_COMMENT,
              _("Welcome message set to the default."));
  } else if (arg[0] != '\0') {
    if (welcome_message) {
      free(welcome_message);
    }
    welcome_message = mystrdup(arg);
    cmd_reply(CMD_WELCOME_MESSAGE, caller, C_COMMENT,
              _("Welcome message changed to \"%s\"."), welcome_message);
  } else {
    if (!welcome_message) {
      cmd_reply(CMD_WELCOME_MESSAGE, caller, C_COMMENT,
                _("The welcome message is set to the default."));
    } else {
      cmd_reply(CMD_WELCOME_MESSAGE, caller, C_COMMENT,
                _("The current welcome message is:\n%s"), welcome_message);
    }
  }
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static bool dnslookup_command(struct connection *caller, char *arg,
                              bool check)
{
  if (check) {
    return TRUE;
  }
  if (!strcmp(arg, "on")) {
    srvarg.no_dns_lookup = FALSE;
  } else if (!strcmp(arg, "off")) {
    srvarg.no_dns_lookup = TRUE;
  } else if (arg[0]) {
    cmd_reply(CMD_DNS_LOOKUP, caller, C_SYNTAX,
              _("The only valid arguments are \"on\" or \"off\"."));
    return FALSE;
  }
  cmd_reply(CMD_DNS_LOOKUP, caller, C_COMMENT, _("DNS lookup is %s."),
            srvarg.no_dns_lookup ? _("disabled") : _("enabled"));
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static void reload_settings(void)
{
  settings_reset();

  /* FIXME: Ugly hack. Find a better way. */
  notify_enabled(FALSE);

  if (srvarg.script_filename &&
      !read_init_script(NULL, srvarg.script_filename)) {
    freelog(LOG_ERROR, _("Cannot load the script file '%s'"),
            srvarg.script_filename);
  }
  notify_enabled(TRUE);
}

/**************************************************************************
  ...
**************************************************************************/
static void show_required_capabilities(struct connection *pconn)
{
  if (srvarg.required_cap[0] != '\0') {
    cmd_reply(CMD_REQUIRE, pconn, C_COMMENT,
              _("Current required capabilities are '%s'"),
              srvarg.required_cap);
  } else {
    cmd_reply(CMD_REQUIRE, pconn, C_COMMENT,
              _("There is no current required capabilities"));
  }
}

/**************************************************************************
  ...
**************************************************************************/
bool require_command(struct connection *caller, char *arg, bool check)
{
  char *cap[256], buf[MAX_LEN_CONSOLE_LINE];
  int ntokens = 0, i;

  if (caller && caller->access_level < ALLOW_ADMIN) {
    show_required_capabilities(caller);
    return TRUE;
  }

  if (arg && strlen(arg) > 0) {
    sz_strlcpy(buf, arg);
    ntokens = get_tokens(buf, cap, 256, TOKEN_DELIMITERS);
  }

  /* Ensure capability is supported exist */
  for (i = 0; i < ntokens; i++) {
    if (!mystrcasecmp(cap[i], "?")) {
      show_required_capabilities(caller);
      return TRUE;
    } else if (!has_capability(cap[i], our_capability)) {
      cmd_reply(CMD_REQUIRE, caller, C_FAIL,
                _("You cannot require the '%s' capability, "
                  "which is not supported by the server."), cap[i]);
      return FALSE;
    }
  }

  if (check) {
    return TRUE;
  }

  srvarg.required_cap[0] = '\0';
  for (i = 0; i < ntokens; i++) {
    cat_snprintf(srvarg.required_cap, sizeof(srvarg.required_cap),
                 "%s%s", i ? " " : "", cap[i]);
  }

  if (srvarg.required_cap[0] != '\0') {
    notify_conn(NULL, _("Server: Required capabilities set to '%s'"),
                srvarg.required_cap);
  } else {
    notify_conn(NULL, _("Server: Required capabilities have been cleared"));
  }

  /* detach all bad connections without this capability */
  conn_list_iterate(game.game_connections, pconn) {
    if (!pconn->observer && !can_control_a_player(pconn, TRUE)) {
      detach_command(pconn, "", FALSE);
    }
  } conn_list_iterate_end;

  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static void kick_table_add(struct connection *pconn)
{
  struct kickinfo *ki;

  if (!kick_table
      || hash_key_exists(kick_table, pconn->server.ipaddr)) {
    return;
  }

  ki = fc_calloc(1, sizeof(*ki));
  ki->time_of_kick = time(NULL);
  sz_strlcpy(ki->addr, pconn->server.ipaddr);
  hash_insert(kick_table, ki->addr, ki);
}

/**************************************************************************
...
**************************************************************************/
static void kick_table_remove(struct connection *pconn)
{
  struct kickinfo *ki;

  if (!kick_table) {
    return;
  }

  ki = hash_delete_entry(kick_table, pconn->server.ipaddr);

  if (ki) {
    free(ki);
  }
}

/**************************************************************************
  ...
**************************************************************************/
bool conn_is_kicked(struct connection *pconn, int *time_remaining)
{
  struct kickinfo *ki;
  time_t now;

  if (!kick_table) {
    return FALSE;
  }

  if (!pconn) {
    return FALSE;
  }

  ki = hash_lookup_data(kick_table, pconn->server.ipaddr);
  
  if (!ki) {
    return FALSE;
  }

  now = time(NULL);
  if (now - ki->time_of_kick > game.server.kicktime) {
    kick_table_remove(pconn);
    return FALSE;
  }

  if (time_remaining) {
    *time_remaining = game.server.kicktime - (now - ki->time_of_kick);
  }
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static bool kick_command(struct connection *caller, char *name, bool check)
{
  struct connection *pconn;
  struct player *pplayer;
  enum m_pre_result match_result;
  bool was_connected;
  char ipaddr[MAX_LEN_ADDR];
  struct conn_list *kick_list;
  struct hash_table *unique_ipaddr_table;
  int num_unique_connections;
  const int MIN_UNIQUE_CONNS = 3;
  
  pconn = find_conn_by_user_prefix(name, &match_result);
  if (!pconn) {
    cmd_reply_no_such_conn(CMD_KICK, caller, name, match_result);
    return FALSE;
  }

  if (caller != NULL && caller->access_level < ALLOW_ADMIN) {
    if (pconn == caller) {
      cmd_reply(CMD_KICK, caller, C_FAIL,
                _("You may not kick yourself."));
      return FALSE;
    }

    unique_ipaddr_table = hash_new(hash_fval_string2, hash_fcmp_string);
    conn_list_iterate(game.est_connections, pc) {
      hash_insert(unique_ipaddr_table, pc->server.ipaddr, NULL);
    } conn_list_iterate_end;

    num_unique_connections = hash_num_entries(unique_ipaddr_table); 
    hash_free(unique_ipaddr_table);

    if (num_unique_connections < MIN_UNIQUE_CONNS) {
      cmd_reply(CMD_KICK, caller, C_FAIL,
                _("There must be at least %d unique connections "
                  "to the server for this command to be valid."),
                MIN_UNIQUE_CONNS);
      return FALSE;
    }
  }
  
  if (check) {
    return TRUE;
  }

  sz_strlcpy(ipaddr, pconn->server.ipaddr);
  kick_list = conn_list_new();

  conn_list_iterate(game.all_connections, pc) {
    if (0 == mystrncasecmp(ipaddr, pc->server.ipaddr, MAX_LEN_ADDR)) {
      conn_list_append(kick_list, pc);
    }
  } conn_list_iterate_end;

  conn_list_iterate(kick_list, pc) {
    pplayer = pc->player;
    was_connected = pplayer ? pplayer->is_connected : FALSE;
    kick_table_add(pc);
    server_break_connection(pc, ES_KICKED);

    if (pplayer && was_connected && !pplayer->is_connected) {
      sz_strlcpy(pplayer->username, ANON_USER_NAME);
    }
  } conn_list_iterate_end;

  conn_list_free(kick_list);

  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static bool welcome_file_command(struct connection *caller,
                                 char *arg, bool check)
{
  FILE *f = NULL;
  long len = -1;
  char *buf = NULL;
  if (check)
    return TRUE;
  if (arg[0] == '\0') {
    cmd_reply(CMD_WELCOME_FILE, caller, C_SYNTAX,
              _("This command requires an argument."));
    return FALSE;
  }
  if (!(f = fopen(arg, "r"))) {
    cmd_reply(CMD_WELCOME_FILE, caller, C_GENFAIL,
              _("Could not open welcome file \"%s\" for reading: %s."),
              arg, mystrerror(myerrno()));
    return FALSE;
  }
  len = get_file_size(arg);
  if (len == -1) {
    cmd_reply(CMD_WELCOME_FILE, caller, C_GENFAIL,
              _("Could not get size of file \"%s\": %s."),
              arg, mystrerror(myerrno()));
    fclose(f);
    return FALSE;
  }
  if (len == 0) {
    buf = fc_malloc(1);
    buf[0] = '\0';
  } else {
    int nb;
    buf = fc_malloc(len + 1);
    nb = fread(buf, 1, len, f);
    if (ferror(f) || nb != len) {
      cmd_reply(CMD_WELCOME_FILE, caller, C_GENFAIL,
                _("Error while reading from \"%s\": %s."),
                arg, mystrerror(myerrno()));
      free(buf);
      fclose(f);
      return FALSE;
    }
    fclose(f);
    buf[len] = '\0';
    /* Remove trailing newline (or "\r\n") since notify_conn
     *  will print it for us. */
    if (len > 1 && buf[len - 1] == '\n') {
      buf[len - 1] = '\0';
      len--;
      if (len > 1 && buf[len - 1] == '\r') {
        buf[len - 1] = '\0';
        len--;
      }
    }
  }
  if (welcome_message) {
    free(welcome_message);
  }
  welcome_message = buf;
  cmd_reply(CMD_WELCOME_FILE, caller, C_COMMENT,
            _("Welcome message set to contents of \"%s\" (%ld bytes)."),
            arg, len);
  return TRUE;
}

/**************************************************************************
  Common code for meta* related commands to reduce code duplication.
  Argument cmd_id gives the command we are running as.
**************************************************************************/
static bool do_meta_set_command(struct connection *caller,
                                const char *arg, bool check, int cmd_id)
{
  char buf[1024];
  bool meta_open = FALSE;

  if (check) {
    return TRUE;
  }

  sz_strlcpy(buf, arg);
  remove_leading_trailing_spaces(buf);

  if (buf[0] == '\0') {
    if (cmd_id == CMD_METAMESSAGE) {
      cmd_reply(cmd_id, caller, C_COMMENT,
                _("Metamessage is \"%s\"."), get_meta_message_string());
    } else if (cmd_id == CMD_METATOPIC) {
      cmd_reply(cmd_id, caller, C_COMMENT,
                _("Metatopic is \"%s\"."), get_meta_topic_string());
    } else if (cmd_id == CMD_METAPATCHES) {
      cmd_reply(cmd_id, caller, C_COMMENT,
                _("Metaserver patches string is \"%s\"."),
                get_meta_patches_string());
    }
    return TRUE;
  }

  if (cmd_id == CMD_METAMESSAGE) {
    set_user_meta_message_string(buf);
  } else if (cmd_id == CMD_METATOPIC) {
    set_meta_topic_string(buf);
  } else if (cmd_id == CMD_METAPATCHES) {
    set_meta_patches_string(buf);
  }

  meta_open = is_metaserver_open();

  if (meta_open) {
    send_server_info_to_metaserver(META_INFO);
  }

  /* In order to avoid untranslatable sentence fragments in
   * the following code, we avoid using a %s format for the 
   * meta* thing that is being modified. */

  if (cmd_id == CMD_METAMESSAGE) {
    if (caller) {
      /* TRANS: Printed when a user sets the metamessage. */
      notify_conn(NULL, _("Server: %s sets the metamessage to '%s'."),
                  caller->username, buf);
    } else {
      notify_conn(NULL, _("Server: Metamessage set to '%s'."), buf);
    }
  } else if (cmd_id == CMD_METATOPIC) {
    if (caller) {
      /* TRANS: Printed when a user sets the metatopic. */
      notify_conn(NULL, _("Server: %s sets the metatopic to '%s'."),
                  caller->username, buf);
    } else {
      notify_conn(NULL, _("Server: Metatopic set to '%s'."), buf);
    }
  } else if (cmd_id == CMD_METAPATCHES) {
    cmd_reply(cmd_id, caller, C_OK,
              _("Metaserver patches string set to '%s'."), buf);
  }

  if (!meta_open && cmd_id != CMD_METAPATCHES) {
    /* TRANS: Don't translate "/metac up". */
    notify_conn(NULL, _("Server: Not reporting to metaserver "
                        "(maybe you need to do /metac up)."));
  }

  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static bool metaserver_command(struct connection *caller, char *arg,
                               bool check)
{
  if (check) {
    return TRUE;
  }
  close_metaserver_connection(caller);
  sz_strlcpy(srvarg.metaserver_addr, arg);
  notify_conn(NULL, _("Metaserver is now [%s]."), meta_addr_port());
  return TRUE;
}

/**************************************************************************
 Returns the serverid 
**************************************************************************/
static bool show_serverid(struct connection *caller, char *arg)
{
  cmd_reply(CMD_SRVID, caller, C_COMMENT, _("Server id: %s"),
            srvarg.serverid);
  return TRUE;
}

/***************************************************************
...
***************************************************************/
static int handicap_of_skill_level(int level)
{
  int h[11] = { -1,
    /* away   */ H_AWAY | H_RATES | H_TARGETS | H_HUTS | H_FOG | H_MAP
        | H_REVOLUTION,
    /* novice */ H_RATES | H_TARGETS | H_HUTS | H_NOPLANES
        | H_DIPLOMAT | H_LIMITEDHUTS | H_DEFENSIVE
        | H_DIPLOMACY | H_REVOLUTION,
    /* easy */ H_RATES | H_TARGETS | H_HUTS | H_NOPLANES
        | H_DIPLOMAT | H_LIMITEDHUTS | H_DEFENSIVE,
    H_NONE,
    /* medium */ H_RATES | H_TARGETS | H_HUTS | H_DIPLOMAT,
    H_NONE,
    /* hard */ H_NONE,
    H_NONE,
    H_NONE,
    /* testing */ H_EXPERIMENTAL,
  };
  assert(level > 0 && level <= 10);
  return h[level];
}

/**************************************************************************
Return the AI fuzziness (0 to 1000) corresponding to a given skill
level (1 to 10).  See ai_fuzzy() in common/player.c
**************************************************************************/
static int fuzzy_of_skill_level(int level)
{
  int f[11] =
      { -1, 0, 400 /*novice */ , 300 /*easy */ , 0, 0, 0, 0, 0, 0, 0 };
  assert(level > 0 && level <= 10);
  return f[level];
}

/**************************************************************************
Return the AI's science development cost; a science development cost of 100
means that the AI develops science at the same speed as a human; a science
development cost of 200 means that the AI develops science at half the speed
of a human, and a sceence development cost of 50 means that the AI develops
science twice as fast as the human
**************************************************************************/
static int science_cost_of_skill_level(int level)
{
  int x[11] =
      { -1, 100, 250 /*novice */ , 100 /*easy */ , 100, 100, 100, 100,
    100, 100, 100
  };
  assert(level > 0 && level <= 10);
  return x[level];
}

/**************************************************************************
Return the AI expansion tendency, a percentage factor to value new cities,
compared to defaults.  0 means _never_ build new cities, > 100 means to
(over?)value them even more than the default (already expansionistic) AI.
**************************************************************************/
static int expansionism_of_skill_level(int level)
{
  int x[11] = { -1, 100, 10 /*novice */ , 10 /*easy */ , 100, 100, 100, 100,
    100, 100, 100
  };
  assert(level > 0 && level <= 10);
  return x[level];
}

/**************************************************************************
For command "save foo";
Save the game, with filename=arg, provided server state is ok.
**************************************************************************/
static bool save_command(struct connection *caller, char *arg, bool check)
{
  if (server_state == SELECT_RACES_STATE) {
    cmd_reply(CMD_SAVE, caller, C_SYNTAX,
              _("The game cannot be saved before it is started."));
    return FALSE;
  } else if (!check) {
    save_game(arg);
  }
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
void toggle_ai_player_direct(struct connection *caller,
                             struct player *pplayer)
{
  assert(pplayer != NULL);
  if (is_barbarian(pplayer)) {
    cmd_reply(CMD_AITOGGLE, caller, C_FAIL,
              _("Cannot toggle a barbarian player."));
    return;
  }
  pplayer->ai.control = !pplayer->ai.control;
  if (pplayer->ai.control) {
    cmd_reply(CMD_AITOGGLE, caller, C_OK,
              _("%s is now under AI control."), pplayer->name);
    if (pplayer->ai.skill_level == 0) {
      pplayer->ai.skill_level = game.info.skill_level;
    }
    /* Set the skill level explicitly, because eg: the player skill
       level could have been set as AI, then toggled, then saved,
       then reloaded. */
    set_ai_level(caller, pplayer->name, pplayer->ai.skill_level, FALSE);
    /* the AI can't do active diplomacy */
    cancel_all_meetings(pplayer);
    /* The following is sometimes necessary to avoid using
       uninitialized data... */
    if (server_state == RUN_GAME_STATE) {
      assess_danger_player(pplayer);
    }
    /* In case this was last player who has not pressed turn done. */
    check_for_full_turn_done();
  } else {
    cmd_reply(CMD_AITOGGLE, caller, C_OK,
              _("%s is now under human control."), pplayer->name);

    /* because the hard AI `cheats' with government rates but humans shouldn't */
    if (!game.server.is_new_game) {
      check_player_government_rates(pplayer);
    }
    /* Remove hidden dialogs from clients. This way the player can initiate
     * new meeting */
    cancel_all_meetings(pplayer);
  }

  if (server_state == RUN_GAME_STATE) {
    send_player_info(pplayer, NULL);
    gamelog(GAMELOG_PLAYER, pplayer);
  }
}

/**************************************************************************
...
**************************************************************************/
static bool toggle_ai_player(struct connection *caller, char *arg,
                             bool check)
{
  enum m_pre_result match_result;
  struct player *pplayer;
  pplayer = find_player_by_name_prefix(arg, &match_result);
  if (!pplayer) {
    cmd_reply_no_such_player(CMD_AITOGGLE, caller, arg, match_result);
    return FALSE;
  } else if (!check) {
    toggle_ai_player_direct(caller, pplayer);
  }
  return TRUE;
}

/****************************************************************************
  Return the number of non-observer players.  game.info.nplayers includes
  observers so in some places this function should be called instead.
****************************************************************************/
static int get_num_nonobserver_players(void)
{
  int nplayers = 0;
  players_iterate(pplayer) {
    if (!pplayer->is_observer) {
      nplayers++;
    }
  } players_iterate_end;
  return nplayers;
}

/**************************************************************************
...
**************************************************************************/
static bool create_ai_player(struct connection *caller, char *arg,
                             bool check)
{
  struct player *pplayer;
  PlayerNameStatus PNameStatus;
  if (server_state != PRE_GAME_STATE) {
    cmd_reply(CMD_CREATE, caller, C_SYNTAX,
              _("Can't add AI players once the game has begun."));
    return FALSE;
  }

  /* game.info.max_players is a limit on the number of non-observer players.
   * MAX_NUM_PLAYERS is a limit on all players. */
  if (get_num_nonobserver_players() >= game.info.max_players
      || game.info.nplayers >= MAX_NUM_PLAYERS) {
    cmd_reply(CMD_CREATE, caller, C_FAIL,
              _("Can't add more players, server is full."));
    return FALSE;
  }

  if ((PNameStatus = test_player_name(arg)) == PNameEmpty) {
    cmd_reply(CMD_CREATE, caller, C_SYNTAX, _("Can't use an empty name."));
    return FALSE;
  }

  if (PNameStatus == PNameTooLong) {
    cmd_reply(CMD_CREATE, caller, C_SYNTAX,
              _("That name exceeds the maximum of %d chars."),
              MAX_LEN_NAME - 1);
    return FALSE;
  }
  if (PNameStatus == PNameIllegal) {
    cmd_reply(CMD_CREATE, caller, C_SYNTAX, _("That name is not allowed."));
    return FALSE;
  }
  if ((pplayer = find_player_by_name(arg))) {
    cmd_reply(CMD_CREATE, caller, C_BOUNCE,
              _("A player already exists by that name."));
    return FALSE;
  }
  if ((pplayer = find_player_by_user(arg))) {
    cmd_reply(CMD_CREATE, caller, C_BOUNCE,
              _("A user already exists by that name."));
    return FALSE;
  }
  if (check) {
    return TRUE;
  }
  pplayer = &game.players[game.info.nplayers];
  server_player_init(pplayer, FALSE);
  sz_strlcpy(pplayer->name, arg);
  sz_strlcpy(pplayer->username, ANON_USER_NAME);
  pplayer->was_created = TRUE;  /* must use /remove explicitly to remove */

  game.info.nplayers++;

  notify_conn(NULL,
              _("Game: %s has been added as an AI-controlled player."),
              arg);

  pplayer = find_player_by_name(arg);
  if (!pplayer) {
    cmd_reply(CMD_CREATE, caller, C_FAIL,
              _("Error creating new AI player: %s."), arg);
    return FALSE;
  }

  pplayer->ai.control = TRUE;
  set_ai_level_directer(pplayer, game.info.skill_level);
  (void) send_server_info_to_metaserver(META_INFO);
  return TRUE;
}


/**************************************************************************
...
**************************************************************************/
static bool remove_player(struct connection *caller, char *arg, bool check)
{
  enum m_pre_result match_result;
  struct player *pplayer;
  char name[MAX_LEN_NAME];
  pplayer = find_player_by_name_prefix(arg, &match_result);
  if (!pplayer) {
    cmd_reply_no_such_player(CMD_REMOVE, caller, arg, match_result);
    return FALSE;
  }
  if (server_state == GAME_OVER_STATE) {
    cmd_reply(CMD_REMOVE, caller, C_FAIL,
              _("You cannot remove player after the game ended."));
    return FALSE;
  }
  sz_strlcpy(name, pplayer->name);
  if (!(game.server.is_new_game && (server_state == PRE_GAME_STATE ||
                                    server_state == SELECT_RACES_STATE))) {
    cmd_reply(CMD_REMOVE, caller, C_OK,
              _("Removed player %s from the game."), name);
    pplayer->is_dying = TRUE;
    kill_player(pplayer);
    return TRUE;
  }
  if (check) {
    return TRUE;
  }
  team_remove_player(pplayer);
  server_remove_player(pplayer);
  if (!caller || caller->used) {  /* may have removed self */
    cmd_reply(CMD_REMOVE, caller, C_OK,
              _("Removed player %s from the game."), name);
  }
  return TRUE;
}

/**************************************************************************
  Returns FALSE iff there was an error.
**************************************************************************/
bool read_init_script(struct connection * caller, char *script_filename)
{
  FILE *script_file;
  char real_filename[1024];

  freelog(LOG_NORMAL, _("Loading script file: %s"), script_filename);

  interpret_tilde(real_filename, sizeof(real_filename), script_filename);

  if (is_reg_file_for_access(real_filename, FALSE)
      && (script_file = fopen(real_filename, "r"))) {
    char buffer[MAX_LEN_CONSOLE_LINE];
    /* the size is set as to not overflow buffer in handle_stdin_input */
    while (fgets(buffer, MAX_LEN_CONSOLE_LINE - 1, script_file)) {
      handle_stdin_input((struct connection *) NULL, buffer, FALSE);
    }
    fclose(script_file);
    return TRUE;

  } else {
    cmd_reply(CMD_READ_SCRIPT, caller, C_FAIL,
              _("Cannot read command line scriptfile '%s'."),
              real_filename);
    freelog(LOG_ERROR, _("Could not read script file '%s'."),
            real_filename);
    return FALSE;
  }
}

/**************************************************************************
...
**************************************************************************/
static bool read_command(struct connection *caller, char *arg, bool check)
{
  if (check) {
    return TRUE;                /* FIXME: no actual checks done */
  }
  /* warning: there is no recursion check! */
  return read_init_script(caller, arg);
}

/**************************************************************************
...
(Should this take a 'caller' argument for output? --dwp)
**************************************************************************/
static void write_init_script(char *script_filename)
{
  char real_filename[1024];
  FILE *script_file;
  interpret_tilde(real_filename, sizeof(real_filename), script_filename);
  if (is_reg_file_for_access(real_filename, TRUE)
      && (script_file = fopen(real_filename, "w"))) {
    int i;
    fprintf(script_file,
            "#FREECIV SERVER COMMAND FILE, version %s\n", VERSION_STRING);
    fputs("# These are server options saved from a running civserver.\n",
          script_file);

    /* first, some state info from commands (we can't save everything) */

    fprintf(script_file, "%s\n",
            (game.info.skill_level == 1) ? "away" :
            (game.info.skill_level == 2) ? "novice" :
            (game.info.skill_level == 3) ? "easy" :
            (game.info.skill_level == 5) ? "medium" :
            (game.info.skill_level < 10) ? "hard" : "experimental");
    if (*srvarg.metaserver_addr != '\0' &&
        ((0 != strcmp(srvarg.metaserver_addr, DEFAULT_META_SERVER_ADDR)))) {
      fprintf(script_file, "metaserver %s\n", meta_addr_port());
    }
    if (0 !=
        strcmp(get_meta_patches_string(), default_meta_patches_string())) {
      fprintf(script_file, "metapatches %s\n", get_meta_patches_string());
    }
    if (0 != strcmp(get_meta_topic_string(), default_meta_topic_string())) {
      fprintf(script_file, "metatopic %s\n", get_meta_topic_string());
    }
    if (0 !=
        strcmp(get_meta_message_string(), default_meta_message_string())) {
      fprintf(script_file, "metamessage %s\n", get_meta_message_string());
    }
    /* then, the 'set' option settings */
    for (i = 0; settings[i].name; i++) {
      struct settings_s *op = &settings[i];
      switch (op->type) {
      case SSET_INT:
        fprintf(script_file, "set %s %i\n", op->name, *op->int_value);
        break;
      case SSET_BOOL:
        fprintf(script_file, "set %s %i\n", op->name,
                (*op->bool_value) ? 1 : 0);
        break;
      case SSET_STRING:
        fprintf(script_file, "set %s %s\n", op->name, op->string_value);
        break;
      }
    }
    /* rulesetdir */
    fprintf(script_file, "rulesetdir %s\n", game.server.rulesetdir);
    fclose(script_file);
  } else {
    freelog(LOG_ERROR,
            _("Could not write script file '%s'."), real_filename);
  }
}

/**************************************************************************
...
('caller' argument is unused)
**************************************************************************/
static bool write_command(struct connection *caller, char *arg, bool check)
{
  if (!check) {
    write_init_script(arg);
  }
  return TRUE;
}

/**************************************************************************
  Set ptarget's cmdlevel to level if caller is allowed to do so
**************************************************************************/
static bool set_cmdlevel(struct connection *caller,
                         struct connection *ptarget, enum cmdlevel_id level)
{
  assert(ptarget != NULL);      /* Only ever call me for specific connection */

  if (caller && ptarget->access_level > caller->access_level) {
    /*
     * This command is intended to be used at ctrl access level
     * and thus this if clause is needed.
     * (Imagine a ctrl level access player that wants to change
     * access level of a hack level access player)
     * At the moment it can be used only by hack access level 
     * and thus this clause is never used.
     */
    cmd_reply(CMD_CMDLEVEL, caller, C_FAIL,
              _
              ("Cannot decrease command access level '%s' for connection '%s';"
               " you only have '%s'."),
              cmdlevel_name(ptarget->access_level), ptarget->username,
              cmdlevel_name(caller->access_level));
    return FALSE;
  } else {
    ptarget->granted_access_level = ptarget->access_level = level;
    return TRUE;
  }
}

/**************************************************************************
 Change command access level for individual player, or all, or new.
**************************************************************************/
static bool cmdlevel_command(struct connection *caller, char *str,
                             bool check)
{
  char *arg[2];
  int i, ntokens;
  bool ret = FALSE;
  enum m_pre_result match_result;
  enum cmdlevel_id level;
  struct connection *ptarget;

  ntokens = get_tokens(str, arg, 2, TOKEN_DELIMITERS);

  if (ntokens == 0) {
    /* No argument supplied; list the levels */
    cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT, horiz_line);
    cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT,
              _("Command access levels in effect:"));
    cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT, horiz_line);
    conn_list_iterate(game.est_connections, pconn) {
      cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT, "cmdlevel %s %s",
                cmdlevel_name(pconn->access_level), pconn->username);
    } conn_list_iterate_end;
    cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT, horiz_line);
    return TRUE;
  }

  /* A level name was supplied; set the level */
  if ((level = cmdlevel_named(arg[0])) == ALLOW_UNRECOGNIZED) {
    char buf[512];
    int i;

    sz_strlcpy(buf, _("Error: command access level must be one of "));
    for (i = 0; i < ALLOW_NUM; i++) {
      cat_snprintf(buf, sizeof(buf), "%s'%s'%s",
                   i == ALLOW_NUM - 1 ? _("or ") : "",
                   cmdlevel_name(i), i == ALLOW_NUM - 1 ? "." : ", ");
    }
    cmd_reply(CMD_CMDLEVEL, caller, C_SYNTAX, buf);
    goto end;
  } else if (caller && level > caller->access_level) {
    cmd_reply(CMD_CMDLEVEL, caller, C_FAIL,
              _("Cannot increase command access level to '%s';"
                " you only have '%s' yourself."),
              arg[0], cmdlevel_name(caller->access_level));
    goto end;
  }

  if (check) {
    return TRUE;                /* looks good */
  }

  if (ntokens == 1) {
    /* No playername supplied: set for all connections */
    conn_list_iterate(game.est_connections, pconn) {
      if (set_cmdlevel(caller, pconn, level)) {
        cmd_reply(CMD_CMDLEVEL, caller, C_OK,
                  _("Command access level set to '%s' for connection %s."),
                  cmdlevel_name(level), pconn->username);
        ret = TRUE;
      } else {
        cmd_reply(CMD_CMDLEVEL, caller, C_FAIL,
                  _("Command access level could not be set to '%s' for "
                    "connection %s."),
                  cmdlevel_name(level), pconn->username);
      }
    } conn_list_iterate_end;
  } else if ((ptarget = find_conn_by_user_prefix(arg[1], &match_result))) {
    if (set_cmdlevel(caller, ptarget, level)) {
      cmd_reply(CMD_CMDLEVEL, caller, C_OK,
                _("Command access level set to '%s' for connection %s."),
                cmdlevel_name(level), ptarget->username);
      ret = TRUE;
    } else {
      cmd_reply(CMD_CMDLEVEL, caller, C_FAIL,
                _("Command access level could not be set to '%s'"
                  " for connection %s."),
                cmdlevel_name(level), ptarget->username);
    }
  } else {
    cmd_reply_no_such_conn(CMD_CMDLEVEL, caller, arg[1], match_result);
  }

end:
  for (i = 0; i < ntokens; i++) {
    free(arg[i]);
  }
  return ret;
}

/**************************************************************************
  Returns possible parameters for the commands that take server options
  as parameters (CMD_EXPLAIN and CMD_SET).
**************************************************************************/
static const char *optname_accessor(int i)
{
  return settings[i].name;
}

#if defined HAVE_LIBREADLINE || defined HAVE_NEWLIBREADLINE
/**************************************************************************
  Returns possible parameters for the /show command.
**************************************************************************/
static const char *olvlname_accessor(int i)
{
  /* for 0->4, uses option levels, otherwise returns a setting name */
  if (i < SSET_NUM_LEVELS) {
    return sset_level_names[i];
  } else {
    return settings[i - SSET_NUM_LEVELS].name;
  }
}
#endif

/**************************************************************************
  ...
**************************************************************************/
static bool ignore_command(struct connection *caller, char *str, bool check)
{
  char buf[128], pat[128], err[64];
  int type, n;
  struct conn_pattern *ap;
  if (!caller) {
    cmd_reply(CMD_IGNORE, caller, C_FAIL,
              _("That would be rather silly, since you are not a player."));
    return FALSE;
  }
  sz_strlcpy(buf, str);
  remove_leading_trailing_spaces(buf);

  type = CPT_USERNAME;
  if (!parse_conn_pattern(buf, pat, sizeof(pat), &type, err, sizeof(err))) {
    cmd_reply(CMD_IGNORE, caller, C_SYNTAX,
              _("Incorrect pattern syntax: %s. Try /help ignore."), err);
    return FALSE;
  }
  if (check)
    return TRUE;

  ap = conn_pattern_new(pat, type);
  ignore_list_append(caller->server.ignore_list, ap);
  n = ignore_list_size(caller->server.ignore_list);
  conn_pattern_as_str(ap, buf, sizeof(buf));
  cmd_reply(CMD_IGNORE, caller, C_COMMENT,
            _("Added pattern %s as entry %d to your ignore list."), buf, n);

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool parse_range(const char *s, int *first, int *last, char *err,
                        int errlen)
{
  char buf[128], *p;
  const char *numstr;
  int a = 0, b;
  assert(s != NULL);

  for (p = buf; *s && *s != '-'; *p++ = *s++) {
    if (my_isspace(*s)) {
      continue;
    }
    if (!my_isdigit(*s)) {
      my_snprintf(err, errlen, _("'%c' is not a number"), *s);
      return FALSE;
    }
  }
  *p++ = 0;

  if (my_isdigit(buf[0])) {
    a = atoi(buf);
    if (first) {
      *first = a;
    }
  }

  if (!*s) {
    if (my_isdigit(buf[0]) && last) {
      *last = a;
    }
    return TRUE;
  }

  numstr = ++s;
  while (*numstr && my_isspace(*numstr)) {
    numstr++;
  }

  for (s = numstr; *s; s++) {
    if (my_isspace(*s)) {
      continue;
    }
    if (!my_isdigit(*s)) {
      my_snprintf(err, errlen, _("'%c' is not a number"), *s);
      return FALSE;
    }
  }

  if (my_isdigit(*numstr)) {
    b = atoi(numstr);
    if (last) {
      *last = b;
    }
  }
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool unignore_command(struct connection *caller,
                             char *str, bool check)
{
  char arg[128], err[64];
  int first, last, n;

  if (!caller) {
    cmd_reply(CMD_IGNORE, caller, C_FAIL,
              _("That would be rather silly, since you are not a player."));
    return FALSE;
  }

  sz_strlcpy(arg, str);
  remove_leading_trailing_spaces(arg);
  n = ignore_list_size(caller->server.ignore_list);

  if (n == 0) {
    cmd_reply(CMD_UNIGNORE, caller, C_COMMENT,
              _("Your ignore list is empty."));
    return FALSE;
  }

  first = 1;
  last = n;
  if (!parse_range(arg, &first, &last, err, sizeof(err))) {
    cmd_reply(CMD_UNIGNORE, caller, C_SYNTAX,
              _("Range syntax error: %s."), err);
    return FALSE;
  }

  if (!(1 <= first && first <= last && last <= n)) {
    cmd_reply(CMD_UNIGNORE, caller, C_FAIL,
              _("Invalid range: %d to %d."), first, last);
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  n = 1;
  ignore_list_iterate(caller->server.ignore_list, ap) {
    if (first <= n && n <= last) {
      char buf[128];
      conn_pattern_as_str(ap, buf, sizeof(buf));
      cmd_reply(CMD_UNIGNORE, caller, C_COMMENT,
                _("Removed pattern %d (%s) from your ignore list."),
                n, buf);
      ignore_list_unlink(caller->server.ignore_list, ap);
      conn_pattern_free(ap);
      break;
    }
    n++;
  } ignore_list_iterate_end;

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static void free_team_names(char **p, int n)
{
  int i;
  if (!p)
    return;

  for (i = 0; i < n; i++) {
    if (p[i])
      free(p[i]);
  }
  free(p);
}

/**************************************************************************
  Result must be freed with free_team_names.
**************************************************************************/
static char **create_team_names(int n)
{
  int i, r;
  char buf[256];
  char **p = fc_malloc(n * sizeof(char *));
  if (n <= 0)
    return NULL;
  if (n == 2) {
    r = myrand(sizeof(two_team_suggestions) / (n * sizeof(char *)));
    for (i = 0; i < n; i++)
      p[i] = mystrdup(two_team_suggestions[r][i]);
  } else if (n == 3) {
    r = myrand(sizeof(three_team_suggestions) / (n * sizeof(char *)));
    for (i = 0; i < n; i++)
      p[i] = mystrdup(three_team_suggestions[r][i]);
  } else if (n == 4) {
    r = myrand(sizeof(four_team_suggestions) / (n * sizeof(char *)));
    for (i = 0; i < n; i++)
      p[i] = mystrdup(four_team_suggestions[r][i]);
  } else {
    for (i = 0; i < n; i++) {
      my_snprintf(buf, sizeof(buf), "TEAM-%d", i + 1);
      p[i] = mystrdup(buf);
    }
  }
  return p;
}

#ifdef HAVE_MYSQL
/**************************************************************************
  ...
**************************************************************************/
static int autoteam_rated_player_cmp(const void *va, const void *vb)
{
  const struct player *a, *b;
  a = *(const struct player **) va;
  b = *(const struct player **) vb;

  return b->fcdb.rating - a->fcdb.rating;
}
#endif

/**************************************************************************
  ...
**************************************************************************/
static bool autoteam_command(struct connection *caller, char *str,
                             bool check)
{
  char *p, *q, buf[1024], **team_names = NULL;
  int n, i, num, t, step;
  int num_unordered = 0;
  struct player *player_ordering[MAX_NUM_PLAYERS];
  bool player_ordered[MAX_NUM_PLAYERS];
  enum m_pre_result result;
  int ntokens;
  char *args[3];
  bool default_by_rating = FALSE;

  if (server_state != PRE_GAME_STATE || !game.server.is_new_game) {
    cmd_reply(CMD_AUTOTEAM, caller, C_SYNTAX,
              _("Cannot change teams once game has begun."));
    return FALSE;
  }

  ntokens = get_tokens_full(str, args, 3, TOKEN_DELIMITERS, TRUE);

  if (ntokens < 1) {
    cmd_reply(CMD_AUTOTEAM, caller, C_SYNTAX,
              _("Missing number argument. See /help autoteam."));
    free_tokens(args, ntokens);
    return FALSE;
  }

  for (p = args[0]; *p; p++) {
    if (!my_isdigit(*p)) {
      cmd_reply(CMD_AUTOTEAM, caller, C_SYNTAX,
                _("The first argument must be an non-negative integer."));
      free_tokens(args, ntokens);
      return FALSE;
    }
  }

  n = atoi(args[0]);
  if (n < 0 || n > game.info.nplayers) {
    cmd_reply(CMD_AUTOTEAM, caller, C_SYNTAX,
              _("Invalid number argument. See /help autoteam."));
    free_tokens(args, ntokens);
    return FALSE;
  }

  if (n == 0) {
    if (!check) {
      team_clear_teams();
      notify_conn(NULL, _("Server: Teams cleared."));
    }
    free_tokens(args, ntokens);
    return TRUE;
  }

  memset(player_ordered, 0, sizeof(player_ordered));
  num = 0;

#ifdef HAVE_MYSQL
  default_by_rating = TRUE;
#endif

  if ((ntokens == 1 && default_by_rating)
      || (ntokens == 2 && (0 == strcmp(args[1], "rating")
                           || 0 == strcmp(args[1], "ranking")
                           || 0 == strcmp(args[1], "rated")
                           || 0 == strcmp(args[1], "rank")))) {

#ifdef HAVE_MYSQL
    int num_unreliable = 0;

    if (!srvarg.fcdb.enabled) {
      cmd_reply(CMD_AUTOTEAM, caller, C_GENFAIL,
                _("Cannot use ratings to assign teams because "
                  "database support is not enabled."));
      free_tokens(args, ntokens);
      return FALSE;
    }

    if (check) {
      free_tokens(args, ntokens);
      return TRUE;
    }

    if (!fcdb_load_player_ratings(GT_TEAM, FALSE)) {
      cmd_reply(CMD_AUTOTEAM, caller, C_GENFAIL,
                _("There was an error loading ratings from the database."));
      free_tokens(args, ntokens);
      return FALSE;
    }

    players_iterate(pplayer) {
      if (pplayer->fcdb.rating <= 0.0) {
        continue;
      }

      if (pplayer->fcdb.rating_deviation > RATING_CONSTANT_RELIABLE_RD) {
        /* If the player's RD is too high (i.e. they haven't played
         * enough recent games for their real rating to be reliably
         * determined), then don't use their rating for the ordering. */
        num_unreliable++;
        continue;
      }

      player_ordering[num++] = pplayer;
      player_ordered[pplayer->player_no] = TRUE;
    } players_iterate_end;

    if (num > 1) {
      qsort(player_ordering, num, sizeof(struct player *),
            autoteam_rated_player_cmp);
    }
#ifdef DEBUG
    freelog(LOG_DEBUG, "autoteam by rating, ordering after sort:");
    for (i = 0; i < num; i++) {
      freelog(LOG_DEBUG, "  %d: %p %s %f RD=%f", i, player_ordering[i],
              player_ordering[i]->name, player_ordering[i]->fcdb.rating,
              player_ordering[i]->fcdb.rating_deviation);
    }
#endif

    notify_conn(NULL, _("Server: Ordering players by their TEAM rating."));
    if (num_unreliable > 0) {
      notify_conn(NULL, _("Server: %d %s not used for ordering "
                          "because of unreliability (RD > %.0f)."),
                  num_unreliable,
                  PL_("rating", "ratings", num_unreliable),
                  RATING_CONSTANT_RELIABLE_RD);
    }
#else
    cmd_reply(CMD_AUTOTEAM, caller, C_GENFAIL,
              _("Cannot use ratings to assign teams because "
                "database support is not enabled."));
    free_tokens(args, ntokens);
    return FALSE;

#endif /* HAVE_MYSQL */

  } else if ((ntokens == 1 && !default_by_rating)
             || (ntokens >= 2 && 0 == strcmp(args[1], "list"))) {

    p = ntokens > 2 ? args[2] : NULL;
    num = 0;
    for (i = 0; p != NULL && i < MAX_NUM_PLAYERS; i++) {
      struct player *pplayer;

      while (*p && my_isspace(*p)) {
        p++;
      }
      for (q = buf; *p && *p != ';';) {
        *q++ = *p++;
      }
      if (*p == ';') {
        p++;
      }
      *q++ = 0;
      if (!*buf) {
        break;
      }

      pplayer = find_player_by_name_prefix(buf, &result);
      if (!pplayer) {
        cmd_reply(CMD_AUTOTEAM, caller, C_FAIL,
                  _("There is no player corresponding to \"%s\"."), buf);
        free_tokens(args, ntokens);
        return FALSE;
      }

      assert(0 <= pplayer->player_no);
      assert(pplayer->player_no < MAX_NUM_PLAYERS);

      if (player_ordered[pplayer->player_no]) {
        cmd_reply(CMD_AUTOTEAM, caller, C_FAIL,
                  _("%s is in the list more than once!"),
                  pplayer->username);
        free_tokens(args, ntokens);
        return FALSE;
      }
      if (pplayer->is_observer) {
        cmd_reply(CMD_AUTOTEAM, caller, C_FAIL,
                  _("You may not assign observers to a team (%s)."),
                  pplayer->username);
        free_tokens(args, ntokens);
        return FALSE;
      }

      player_ordering[i] = pplayer;
      player_ordered[pplayer->player_no] = TRUE;
      num++;
    }

    if (check) {
      free_tokens(args, ntokens);
      return TRUE;
    }

    if (num > 0) {
      notify_conn(NULL, _("Server: Ordering players using the provided "
                          "list of %d %s."),
                  num, PL_("player", "players", num));
    } else {
      notify_conn(NULL, _("Server: Ordering players arbitrarily."));
    }
  } else {
    /* TRANS: do not translate "/help autoteam". */
    cmd_reply(CMD_AUTOTEAM, caller, C_SYNTAX,
              _("Invalid syntax. Please read /help autoteam."));
    free_tokens(args, ntokens);
    return FALSE;
  }

  free_tokens(args, ntokens);

  /* Assign the remaining unordered players. */
  players_iterate(pplayer) {
    if (pplayer->is_observer || is_barbarian(pplayer)
        || player_ordered[pplayer->player_no]) {
      continue;
    }
    player_ordering[num++] = pplayer;
    player_ordered[pplayer->player_no] = TRUE;
    num_unordered++;
  } players_iterate_end;

  /* Shuffle the unordered players so that consecutive
   * calls to autoteam will generate different teams. */
  for (i = 0; i < num_unordered; i++) {
    int pos, cur;
    struct player *tmp;

    cur = num - num_unordered + i;
    pos = cur + myrand(num_unordered - i);
    tmp = player_ordering[cur];
    player_ordering[cur] = player_ordering[pos];
    player_ordering[pos] = tmp;
  }

#ifdef DEBUG
  freelog(LOG_DEBUG, "autoteam num=%d num_unordered=%d",
          num, num_unordered);
  freelog(LOG_DEBUG, "autoteam player ordering:");
  for (i = 0; i < num; i++) {
    freelog(LOG_DEBUG, "  %d: %p %s", i, player_ordering[i],
            player_ordering[i]->name);
  }
#endif

  notify_conn(NULL, _("Server: Assigning all players to %d teams."), n);
  team_names = create_team_names(n);

  for (i = 0, t = 0, step = 1; i < num; i++) {
    team_remove_player(player_ordering[i]);
    team_add_player(player_ordering[i], team_names[t]);

    /* Do the "ABBAABB...", "ABCCBAABCCBA...", etc. assignment. */
    t = t + step;
    if (step == 1) {
      if (t == n) {
        step = -1;
        t = n - 1;
      }
    } else {                    /* step == -1 */
      if (t < 0) {
        step = 1;
        t = 0;
      }
    }
  }

  free_team_names(team_names, n);
  show_teams(caller, TRUE);

  return TRUE;
}

/**************************************************************************
 ...
**************************************************************************/
bool conn_is_muted(struct connection * pconn)
{
  if (!pconn) {
    return FALSE;
  }
  return hash_key_exists(mute_table, pconn->server.ipaddr);
}

/**************************************************************************
  ...
**************************************************************************/
static bool unmute_command(struct connection *caller, char *str, bool check)
{
  enum m_pre_result res;
  struct muteinfo *mi;
  struct connection *pconn;

  pconn = find_conn_by_user_prefix(str, &res);
  if (!pconn) {
    cmd_reply(CMD_UNMUTE, caller, C_FAIL,
              _("The string \"%s\" does not match any user name."), str);
    return FALSE;
  }

  if (!conn_is_muted(pconn)) {
    cmd_reply(CMD_UNMUTE, caller, C_FAIL, _("User %s is not muted."),
              pconn->username);
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  /* Cannot fail, since conn_is_muted returned true. */
  mi = hash_lookup_data(mute_table, pconn->server.ipaddr);
  assert(mi != NULL);
  unmute_conn_by_mi(mi);

  cmd_reply(CMD_UNMUTE, caller, C_OK, _("User %s has been unmuted."),
            pconn->username);

  send_updated_vote_totals(NULL);

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool mute_command(struct connection *caller, char *str, bool check)
{
  char buf[MAX_LEN_CONSOLE_LINE];
  char *arg[2], *p;
  int ntokens = 0, nturns = 3;
  struct connection *pconn = NULL;
  enum m_pre_result res;
  struct muteinfo *mi;

  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 2, TOKEN_DELIMITERS);

  if (ntokens < 1 || ntokens > 2) {
    cmd_reply(CMD_MUTE, caller, C_SYNTAX, _("Missing username argument."));
    return FALSE;
  }

  pconn = find_conn_by_user_prefix(arg[0], &res);
  if (!pconn) {
    cmd_reply(CMD_MUTE, caller, C_FAIL,
              _("The string \"%s\" does not match any user name."), arg[0]);
    return FALSE;
  }

  if (ntokens == 2) {
    for (p = arg[1]; *p != '\0'; p++) {
      if (!my_isdigit(*p)) {
        cmd_reply(CMD_MUTE, caller, C_SYNTAX,
                  _("Last argument must be an integer."));
        return FALSE;
      }
    }
    nturns = atoi(arg[1]);
    if (nturns < 0) {
      cmd_reply(CMD_MUTE, caller, C_SYNTAX,
                _("Last argument must be a positive integer."));
      return FALSE;
    }
  }

  if (conn_is_muted(pconn)) {
    cmd_reply(CMD_MUTE, caller, C_FAIL,
              _("User %s is already muted."), pconn->username);
    return FALSE;
  }

  if (pconn->access_level >= ALLOW_ADMIN) {
    cmd_reply(CMD_MUTE, caller, C_FAIL,
              _("User %s cannot be muted."), pconn->username);
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  mi = fc_malloc(sizeof(struct muteinfo));
  mi->turns_left = nturns;
  mi->conn_id = pconn->id;
  mi->addr = mystrdup(pconn->server.ipaddr);

  hash_insert(mute_table, mi->addr, mi);

  conn_list_iterate(game.est_connections, pc) {
    if (pc == pconn) {
      if (nturns == 0) {
        notify_conn(pc->self, _("Server: You have been muted."));
      } else {
        notify_conn(pc->self,
                    _("Server: You have been muted for the next %d turns."),
                    nturns);
      }
    } else if (pc == caller) {
      if (nturns == 0) {
        cmd_reply(CMD_MUTE, caller, C_OK, _("User %s has been muted."),
                  pconn->username);
      } else {
        cmd_reply(CMD_MUTE, caller, C_OK,
                  _("User %s has been muted for the next %d turns."),
                  pconn->username, nturns);
      }
    } else {
      if (nturns == 0) {
        notify_conn(pc->self,
                    _("Server: User %s has been muted."), pconn->username);
      } else {
        notify_conn(pc->self,
                    _
                    ("Server: User %s has been muted for the next %d turns."),
                    pconn->username, nturns);
      }
    }
  } conn_list_iterate_end;

  send_updated_vote_totals(NULL);

  return TRUE;
}

/* To allow compililation without mysql and debugging enabled. */
#ifdef HAVE_MYSQL
/**************************************************************************
  ...
**************************************************************************/
static void mkstatdate(time_t t, char *buf, int len)
{
  struct tm tm;
  localtime_r(&t, &tm);
  strftime(buf, len, "%F %T", &tm);
}

/**************************************************************************
  ...
**************************************************************************/
static bool stats_command(struct connection *caller,
                          char *allargs, bool check)
{
  char username[MAX_LEN_NAME + 64], buf[256];
  struct fcdb_user_stats *fus;
  struct game_type_stats *gts;
  int i, recent[5], num_recent = ARRAY_SIZE(recent);
  struct string_list *username_matchs = NULL;

  if (!srvarg.fcdb.enabled) {
    cmd_reply(CMD_STATS, caller, C_FAIL,
              _("Database communication has been disabled."));
    return FALSE;
  }

  sz_strlcpy(username, allargs);
  remove_leading_trailing_spaces(username);

  if (username[0] == '\0') {
    if (caller == NULL) {
      /* Server operator is not a user. */
      cmd_reply(CMD_STATS, caller, C_FAIL,
                _("You are not a user, you are the server operator!"));
      return FALSE;
    }
    sz_strlcpy(username, caller->username);
  }

  if (check) {
    return TRUE;
  }

  username_matchs = string_list_new();

  if (!(fus = fcdb_user_stats_new(username, username_matchs))
      || !fcdb_get_recent_games(username, recent, &num_recent)) {
    cmd_reply(CMD_STATS, caller, C_FAIL,
              _("There was an error communicating with the database."));
    string_list_free_all(username_matchs);
    return FALSE;
  }

  if (fus->id <= 0) {
    if (string_list_size(username_matchs) < 2) {
      cmd_reply(CMD_STATS, caller, C_FAIL,
                _("No user named '%s' in the database."), username);
    } else {
      cmd_reply(CMD_STATS, caller, C_FAIL,
                _("The substring '%s' matchs more than one username:"),
                username);
      string_list_iterate(username_matchs, s) {
        cmd_reply(CMD_STATS, caller, C_COMMENT, "%s", s);
      } string_list_iterate_end;
    }
    string_list_free_all(username_matchs);
    fcdb_user_stats_free(fus);
    return FALSE;
  }

  cmd_reply(CMD_STATS, caller, C_COMMENT, horiz_line);
  cmd_reply(CMD_STATS, caller, C_COMMENT, _("Statistics for %s"),
            fus->username);
  cmd_reply(CMD_STATS, caller, C_COMMENT, horiz_line);
  if (fus->email[0] != '\0') {
    cmd_reply(CMD_STATS, caller, C_COMMENT, _("  Email: %s"), fus->email);
  }

  cmd_reply(CMD_STATS, caller, C_COMMENT,
            _("  User Id: %-24d Login Count: %d"),
            fus->id, fus->logincount);
  mkstatdate(fus->createtime, buf, sizeof(buf));
  cmd_reply(CMD_STATS, caller, C_COMMENT, _("  Created: %-24s From: %s"),
            buf, fus->createaddress);
  mkstatdate(fus->accesstime, buf, sizeof(buf));
  cmd_reply(CMD_STATS, caller, C_COMMENT, _("  Last Login: %-21s From: %s"),
            buf, fus->address);
  cmd_reply(CMD_STATS, caller, C_COMMENT, horiz_line);

  if (fus->num_game_types > 0) {
    cmd_reply(CMD_STATS, caller, C_COMMENT,
              "%-10s %5s %6s %6s %13s %10s %8s",
              _("Game Type"), _("Wins"), _("Loses"), _("Draws"),
              _("Total Played"), _("Rating"), _("RD"));
    for (i = 0; i < fus->num_game_types; i++) {
      gts = &fus->gt_stats[i];
      cmd_reply(CMD_STATS, caller, C_COMMENT,
                "%-10s %5d %6d %6d %13d %10.2f %8.2f",
                gts->type, gts->wins, gts->loses, gts->draws,
                gts->count, gts->rating, gts->rating_deviation);
    }
    cmd_reply(CMD_STATS, caller, C_COMMENT, horiz_line);
  }
  fcdb_user_stats_free(fus);
  string_list_free_all(username_matchs);

  if (num_recent > 0) {
    my_snprintf(buf, sizeof(buf), _("Recent rated games:"));
    for (i = 0; i < num_recent; i++) {
      cat_snprintf(buf, sizeof(buf), "  %d", recent[i]);
    }
    cmd_reply(CMD_STATS, caller, C_COMMENT, "%s", buf);
  }

  return TRUE;
}
#endif /* HAVE_MYSQL */

/**************************************************************************
  Set timeout options.
**************************************************************************/
static bool incremental_timeout_command(struct connection *caller,
                                        char *str,
                                        bool check)
{
  char buf[MAX_LEN_CONSOLE_LINE];
  char *arg[4];
  int i = 0, ntokens;
  int *timeouts[4];

  timeouts[0] = &game.server.timeoutint;
  timeouts[1] = &game.server.timeoutintinc;
  timeouts[2] = &game.server.timeoutinc;
  timeouts[3] = &game.server.timeoutincmult;
  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 4, TOKEN_DELIMITERS);
  for (i = 0; i < ntokens; i++) {
    if (sscanf(arg[i], "%d", timeouts[i]) != 1) {
      cmd_reply(CMD_INCREMENTALTIMEOUT, caller, C_FAIL,
                _("Invalid argument %d."), i + 1);
    }
    free(arg[i]);
  }
  if (ntokens == 0) {
    cmd_reply(CMD_INCREMENTALTIMEOUT, caller, C_SYNTAX,
              _("Usage: incrementaltimeout <turn> <turnadd> "
                "<value> <valuemult>."));
    return FALSE;
  } else if (check) {
    return TRUE;
  }
  cmd_reply(CMD_INCREMENTALTIMEOUT, caller, C_OK,
            _("Dynamic timeout set to %d %d %d %d"),
            game.server.timeoutint, game.server.timeoutintinc,
            game.server.timeoutinc, game.server.timeoutincmult);

  /* if we set anything here, reset the counter */
  game.server.timeoutcounter = 1;
  return TRUE;
}

/**************************************************************************
Find option level number by name.
**************************************************************************/
static enum sset_level lookup_option_level(const char *name)
{
  enum sset_level i;

  for (i = SSET_ALL; i < SSET_NUM_LEVELS; i++) {
    if (0 == mystrcasecmp(name, sset_level_names[i])) {
      return i;
    }
  }

  return SSET_NONE;
}

/**************************************************************************
Find option index by name. Return index (>=0) on success, -1 if no
suitable options were found, -2 if several matches were found.
**************************************************************************/
static int lookup_option(const char *name)
{
  enum m_pre_result result;
  int ind;
  /* Check for option levels, first off */
  if (lookup_option_level(name) != SSET_NONE) {
    return -3;
  }
  result = match_prefix(optname_accessor, SETTINGS_NUM, 0, mystrncasecmp,
                        name, &ind);
  return ((result < M_PRE_AMBIGUOUS) ? ind :
          (result == M_PRE_AMBIGUOUS) ? -2 : -1);
}

/**************************************************************************
 Show the caller detailed help for the single OPTION given by id.
 help_cmd is the command the player used.
 Only show option values for options which the caller can SEE.
**************************************************************************/
static void show_help_option(struct connection *caller,
                             enum command_id help_cmd, int id)
{
  struct settings_s *op = &settings[id];
  if (op->short_help) {
    cmd_reply(help_cmd, caller, C_COMMENT,
              "%s %s  -  %s", _("Option:"), op->name, _(op->short_help));
  } else {
    cmd_reply(help_cmd, caller, C_COMMENT, "%s %s", _("Option:"), op->name);
  }
  if (op->extra_help && strcmp(op->extra_help, "") != 0) {
    static struct astring abuf = ASTRING_INIT;
    const char *help = _(op->extra_help);
    astr_minsize(&abuf, strlen(help) + 1);
    strcpy(abuf.str, help);
    wordwrap_string(abuf.str, 76);
    cmd_reply(help_cmd, caller, C_COMMENT, _("Description:"));
    cmd_reply_prefix(help_cmd, caller, C_COMMENT, "  ", "  %s", abuf.str);
  }
  cmd_reply(help_cmd, caller, C_COMMENT,
            _("Status: %s"), (sset_is_changeable(id)
                              ? _("changeable") : _("fixed")));
  if (op->pregame_level >= 0) {
    cmd_reply(help_cmd, caller, C_COMMENT,
              _("Pregame access level: %s"),
              cmdlevel_name(op->pregame_level));
  }
  if (op->game_level >= 0) {
    cmd_reply(help_cmd, caller, C_COMMENT,
              _("Game access level: %s"),
              cmdlevel_name(op->game_level));
  }
  if (may_view_option(caller, id)) {
    switch (op->type) {
    case SSET_BOOL:
      cmd_reply(help_cmd, caller, C_COMMENT,
                _("Value: %d, Minimum: 0, Default: %d, Maximum: 1"),
                (*(op->bool_value)) ? 1 : 0,
                op->bool_default_value ? 1 : 0);
      break;
    case SSET_INT:
      cmd_reply(help_cmd, caller, C_COMMENT,
                _("Value: %d, Minimum: %d, Default: %d, Maximum: %d"),
                *(op->int_value), op->int_min_value, op->int_default_value,
                op->int_max_value);
      break;
    case SSET_STRING:
      cmd_reply(help_cmd, caller, C_COMMENT,
                _("Value: \"%s\", Default: \"%s\""), op->string_value,
                op->string_default_value);
      break;
    }
  }
}

/**************************************************************************
 Show the caller list of OPTIONS.
 help_cmd is the command the player used.
 Only show options which the caller can SEE.
**************************************************************************/
static void show_help_option_list(struct connection *caller,
                                  enum command_id help_cmd)
{
  int i, j;
  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
  cmd_reply(help_cmd, caller, C_COMMENT,
            _("Explanations are available for the following "
              "server options:"));
  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
  if (!caller && con_get_style()) {
    for (i = 0; settings[i].name; i++) {
      cmd_reply(help_cmd, caller, C_COMMENT, "%s", settings[i].name);
    }
  } else {
    char buf[MAX_LEN_CONSOLE_LINE];
    buf[0] = '\0';
    for (i = 0, j = 0; settings[i].name; i++) {
      if (may_view_option(caller, i)) {
        cat_snprintf(buf, sizeof(buf), "%-19s", settings[i].name);
        if ((++j % 4) == 0) {
          cmd_reply(help_cmd, caller, C_COMMENT, buf);
          buf[0] = '\0';
        }
      }
    }
    if (buf[0] != '\0')
      cmd_reply(help_cmd, caller, C_COMMENT, buf);
  }
  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
}

/**************************************************************************
 ...
**************************************************************************/
static bool explain_option(struct connection *caller, char *str, bool check)
{
  char command[MAX_LEN_CONSOLE_LINE], *cptr_s, *cptr_d;
  int cmd;
  for (cptr_s = str; *cptr_s != '\0' && !my_isalnum(*cptr_s); cptr_s++) {
    /* nothing */
  }
  for (cptr_d = command; *cptr_s != '\0' && my_isalnum(*cptr_s);
       cptr_s++, cptr_d++)
    *cptr_d = *cptr_s;
  *cptr_d = '\0';
  if (*command != '\0') {
    cmd = lookup_option(command);
    if (cmd >= 0 && cmd < SETTINGS_NUM) {
      show_help_option(caller, CMD_EXPLAIN, cmd);
    } else if (cmd == -1 || cmd == -3) {
      cmd_reply(CMD_EXPLAIN, caller, C_FAIL,
                _("No explanation for that yet."));
      return FALSE;
    } else if (cmd == -2) {
      cmd_reply(CMD_EXPLAIN, caller, C_FAIL, _("Ambiguous option name."));
      return FALSE;
    } else {
      freelog(LOG_ERROR, "Unexpected case %d in %s line %d",
              cmd, __FILE__, __LINE__);
      return FALSE;
    }
  } else {
    show_help_option_list(caller, CMD_EXPLAIN);
  }
  return TRUE;
}

/******************************************************************
  Send a message to all players
******************************************************************/
static bool wall(char *str, bool check)
{
  if (!check) {
    notify_conn_ex(game.game_connections, NULL, E_MESSAGE_WALL,
                   _("Server Operator: %s"), str);
  }
  return TRUE;
}

/******************************************************************
Send a report with server options to specified connections.
"which" should be one of:
1: initial options only
2: ongoing options only 
(which=0 means all options; this is now obsolete and no longer used.)
******************************************************************/
void report_server_options(struct conn_list *dest, int which)
{
  int i, c;
  char buffer[4096];
  char title[128];
  const char *caption;
  buffer[0] = '\0';
  my_snprintf(title, sizeof(title), _("%-20svalue  (min , max)"),
              _("Option"));
  caption =
      (which ==
       1) ? _("Server Options (initial)") : _("Server Options (ongoing)");
  for (c = 0; c < SSET_NUM_CATEGORIES; c++) {
    cat_snprintf(buffer, sizeof(buffer), "%s:\n", sset_category_names[c]);
    for (i = 0; settings[i].name; i++) {
      struct settings_s *op = &settings[i];
      if (!sset_is_to_client(i)) {
        continue;
      }
      if (which == 1 && op->sclass > SSET_GAME_INIT) {
        continue;
      }
      if (which == 2 && op->sclass <= SSET_GAME_INIT) {
        continue;
      }
      if (op->category != c) {
        continue;
      }
      switch (op->type) {
      case SSET_BOOL:
        cat_snprintf(buffer, sizeof(buffer), "%-20s%c%-6d (0,1)\n",
                     op->name,
                     (*op->bool_value ==
                      op->bool_default_value) ? '*' : ' ', *op->bool_value);
        break;
      case SSET_INT:
        cat_snprintf(buffer, sizeof(buffer), "%-20s%c%-6d (%d,%d)\n",
                     op->name,
                     (*op->int_value == op->int_default_value) ? '*' : ' ',
                     *op->int_value, op->int_min_value, op->int_max_value);
        break;
      case SSET_STRING:
        cat_snprintf(buffer, sizeof(buffer), "%-20s%c\"%s\"\n", op->name,
                     (strcmp(op->string_value,
                             op->string_default_value) == 0) ? '*' : ' ',
                     op->string_value);
        break;
      }
    }
    cat_snprintf(buffer, sizeof(buffer), "\n");
  }
  freelog(LOG_DEBUG, "report_server_options buffer len %d", i);
  page_conn(dest, caption, title, buffer);
}

/******************************************************************
  Deliver options to the client for setting

  which == 1 : REPORT_SERVER_OPTIONS1
  which == 2 : REPORT_SERVER_OPTIONS2
******************************************************************/
void report_settable_server_options(struct connection *dest, int which)
{
  struct packet_options_settable_control control;
  struct packet_options_settable packet;
  int i, s = 0;
  if (dest->access_level == ALLOW_NONE
      || (which == 1 && server_state > PRE_GAME_STATE)) {
    report_server_options(dest->self, which);
    return;
  }
  memset(&control, 0, sizeof(struct packet_options_settable_control));
  /* count the number of settings */
  for (i = 0; settings[i].name; i++) {
    if (!sset_is_changeable(i)) {
      continue;
    }
    if (settings[i].to_client == SSET_SERVER_ONLY
        && dest->access_level != ALLOW_HACK) {
      continue;
    }
    s++;
  }
  control.nids = s;
  /* fill in the category strings */
  control.ncategories = SSET_NUM_CATEGORIES;
  for (i = 0; i < SSET_NUM_CATEGORIES; i++) {
    strcpy(control.category_names[i], sset_category_names[i]);
  }
  /* send off the control packet */
  send_packet_options_settable_control(dest, &control);
  for (s = 0, i = 0; settings[i].name; i++) {
    if (!sset_is_changeable(i)) {
      continue;
    }
    if (settings[i].to_client == SSET_SERVER_ONLY
        && dest->access_level != ALLOW_HACK) {
      continue;
    }
    memset(&packet, 0, sizeof(packet));

    packet.id = s++;
    sz_strlcpy(packet.name, settings[i].name);
    sz_strlcpy(packet.short_help, settings[i].short_help);
    sz_strlcpy(packet.extra_help, settings[i].extra_help);
    packet.category = settings[i].category;
    packet.type = settings[i].type;
    if (settings[i].type == SSET_STRING) {
      strcpy(packet.strval, settings[i].string_value);
      strcpy(packet.default_strval, settings[i].string_default_value);
    } else if (settings[i].type == SSET_BOOL) {
      packet.val = *(settings[i].bool_value);
      packet.default_val = settings[i].bool_default_value;
    } else {
      packet.min = settings[i].int_min_value;
      packet.max = settings[i].int_max_value;
      packet.val = *(settings[i].int_value);
      packet.default_val = settings[i].int_default_value;
    }

    send_packet_options_settable(dest, &packet);
  }
}

/******************************************************************
  Set an AI level and related quantities, with no feedback.
******************************************************************/
void set_ai_level_directer(struct player *pplayer, int level)
{
  pplayer->ai.handicap = handicap_of_skill_level(level);
  pplayer->ai.fuzzy = fuzzy_of_skill_level(level);
  pplayer->ai.expand = expansionism_of_skill_level(level);
  pplayer->ai.science_cost = science_cost_of_skill_level(level);
  pplayer->ai.skill_level = level;
}

/******************************************************************
  Translate an AI level back to its CMD_* value.
  If we just used /set ailevel <num> we wouldn't have to do this - rp
******************************************************************/
static enum command_id cmd_of_level(int level)
{
  switch (level) {
  case 1:
    return CMD_AWAY;
  case 2:
    return CMD_NOVICE;
  case 3:
    return CMD_EASY;
  case 5:
    return CMD_NORMAL;
  case 7:
    return CMD_HARD;
  case 10:
    return CMD_EXPERIMENTAL;
  }
  assert(FALSE);
  return CMD_NORMAL;            /* to satisfy compiler */
}

/******************************************************************
  Set an AI level from the server prompt.
******************************************************************/
void set_ai_level_direct(struct player *pplayer, int level)
{
  set_ai_level_directer(pplayer, level);
  cmd_reply(cmd_of_level(level), NULL, C_OK,
            _("Player '%s' now has AI skill level '%s'."),
            pplayer->name, name_of_skill_level(level));
}

/******************************************************************
  Handle a user command to set an AI level.
******************************************************************/
static bool set_ai_level(struct connection *caller, char *name,
                         int level, bool check)
{
  enum m_pre_result match_result;
  struct player *pplayer;
//
  assert(level > 0 && level < 11);
  pplayer = find_player_by_name_prefix(name, &match_result);
  if (pplayer) {
    if (pplayer->ai.control) {
      if (check) {
        return TRUE;
      }
      set_ai_level_directer(pplayer, level);
      cmd_reply(cmd_of_level(level), caller, C_OK,
                _("Player '%s' now has AI skill level '%s'."),
                pplayer->name, name_of_skill_level(level));
    } else {
      cmd_reply(cmd_of_level(level), caller, C_FAIL,
                _("%s is not controlled by the AI."), pplayer->name);
      return FALSE;
    }
  } else if (match_result == M_PRE_EMPTY) {
    if (check) {
      return TRUE;
    }
    players_iterate(pplayer) {
      if (pplayer->ai.control) {
        set_ai_level_directer(pplayer, level);
        cmd_reply(cmd_of_level(level), caller, C_OK,
                  _("Player '%s' now has AI skill level '%s'."),
                  pplayer->name, name_of_skill_level(level));
      }
    } players_iterate_end;
    game.info.skill_level = level;
    cmd_reply(cmd_of_level(level), caller, C_OK,
              _("Default AI skill level set to '%s'."),
              name_of_skill_level(level));
  } else {
    cmd_reply_no_such_player(cmd_of_level(level), caller, name,
                             match_result);
    return FALSE;
  }
  return TRUE;
}

/******************************************************************
  Set user to away mode.
******************************************************************/
static bool set_away(struct connection *caller, char *name, bool check)
{
  if (caller == NULL) {
    cmd_reply(CMD_AWAY, caller, C_FAIL, _("This command is client only."));
    return FALSE;
  } else if (name && strlen(name) > 0) {
    notify_conn(caller->self, _("Usage: away"));
    return FALSE;
  } else if (!caller->player || caller->observer) {
    /* This happens for detached or observer connections. */
    notify_conn(caller->self, _("Only players may use the away command."));
    return FALSE;
  } else if (!caller->player->ai.control && !check) {
    notify_conn(game.est_connections, _("%s set to away mode."),
                caller->player->name);
    set_ai_level_directer(caller->player, 1);
    caller->player->ai.control = TRUE;
    cancel_all_meetings(caller->player);
  } else if (!check) {
    notify_conn(game.est_connections, _("%s returned to game."),
                caller->player->name);
    caller->player->ai.control = FALSE;
    /* We have to do it, because the client doesn't display 
     * dialogs for meetings in AI mode. */
    cancel_all_meetings(caller->player);
  }
  return TRUE;
}

/******************************************************************
  ...
******************************************************************/
static bool option_changed(struct settings_s *op)
{
  switch (op->type) {
  case SSET_BOOL:
    return *op->bool_value != op->bool_default_value;
    break;
  case SSET_INT:
    return *op->int_value != op->int_default_value;
    break;
  case SSET_STRING:
    return strcmp(op->string_value, op->string_default_value);
    break;
  default:
    break;
  }
  return FALSE;
}

/******************************************************************
Print a summary of the settings and their values.
Note that most values are at most 4 digits, except seeds,
which we let overflow their columns, plus a sign character.
Only show options which the caller can SEE.
******************************************************************/
static bool show_command(struct connection *caller, char *str, bool check)
{
  char buf[MAX_LEN_CONSOLE_LINE];
  char command[MAX_LEN_CONSOLE_LINE], *cptr_s, *cptr_d;
  int cmd, i, len1, len = 0;
  enum sset_level level = SSET_VITAL;
  size_t clen = 0;
  struct settings_s *op = NULL;
  for (cptr_s = str; *cptr_s != '\0' && !my_isalnum(*cptr_s); cptr_s++) {
    /* nothing */
  }
  for (cptr_d = command; *cptr_s != '\0' && my_isalnum(*cptr_s);
       cptr_s++, cptr_d++)
    *cptr_d = *cptr_s;
  *cptr_d = '\0';
  if (*command != '\0') {
    /* In "/show forests", figure out that it's the forests option we're
     * looking at. */
    cmd = lookup_option(command);
    if (cmd >= 0) {
      /* Ignore levels when a particular option is specified. */
      level = SSET_NONE;
      if (!may_view_option(caller, cmd)) {
        cmd_reply(CMD_SHOW, caller, C_FAIL,
                  _("Sorry, you do not have access to view option '%s'."),
                  command);
        return FALSE;
      }
    }
    if (cmd == -1) {
      cmd_reply(CMD_SHOW, caller, C_FAIL, _("Unknown option '%s'."),
                command);
      return FALSE;
    }
    if (cmd == -2) {
      /* allow ambiguous: show all matching */
      clen = strlen(command);
    }
    if (cmd == -3) {
      /* Option level */
      level = lookup_option_level(command);
    }
  } else {
    cmd = -1;                   /* to indicate that no comannd was specified */
  }
#define cmd_reply_show(string)  cmd_reply(CMD_SHOW, caller, C_COMMENT, string)
#define OPTION_NAME_SPACE 13
  /* under SSET_MAX_LEN, so it fits into 80 cols more easily - rp */
  cmd_reply_show(horiz_line);
  switch (level) {
  case SSET_NONE:
    break;
  case SSET_ALL:
    cmd_reply_show(_("All options"));
    break;
  case SSET_VITAL:
    cmd_reply_show(_("Vital options"));
    break;
  case SSET_SITUATIONAL:
    cmd_reply_show(_("Situational options"));
    break;
  case SSET_RARE:
    cmd_reply_show(_("Rarely used options"));
    break;
  case SSET_CHANGED:
    cmd_reply_show(_("Options changed from the default value"));
    break;
  default:
    break;
  }
  cmd_reply_show(_("+ means you may change the option"));
  if (level != SSET_CHANGED)
    cmd_reply_show(_("= means the option is on its default value"));
  cmd_reply_show(horiz_line);
  len1 = my_snprintf(buf, sizeof(buf),
                     _("%-*s value   (min,max)      "), OPTION_NAME_SPACE,
                     _("Option"));
  if (len1 == -1)
    len1 = sizeof(buf) - 1;
  sz_strlcat(buf, _("description"));
  cmd_reply_show(buf);
  cmd_reply_show(horiz_line);
  buf[0] = '\0';
  for (i = 0; settings[i].name; i++) {
    if (!(may_view_option(caller, i)
          && (cmd == -1 || cmd == -3 || cmd == i
              || (cmd == -2
                  && !mystrncasecmp(settings[i].name, command, clen))))) {
      continue;
    }

    op = &settings[i];
    if (!(level == SSET_ALL || op->level == level || cmd >= 0
          || (level == SSET_CHANGED && option_changed(op)))) {
      continue;
    }
    len = 0;

    switch (op->type) {
    case SSET_BOOL:
      len = my_snprintf(buf, sizeof(buf),
                        "%-*s %c%c%-5d (0,1)", OPTION_NAME_SPACE,
                        op->name, may_set_option_now(caller,
                                                     i) ? '+' : ' ',
                        ((*op->bool_value ==
                          op->bool_default_value) ? '=' : ' '),
                        (*op->bool_value) ? 1 : 0);
      break;
    case SSET_INT:
      len = my_snprintf(buf, sizeof(buf),
                        "%-*s %c%c%-5d (%d,%d)", OPTION_NAME_SPACE,
                        op->name, may_set_option_now(caller,
                                                     i) ? '+' : ' ',
                        ((*op->int_value == op->int_default_value) ?
                         '=' : ' '),
                        *op->int_value, op->int_min_value,
                        op->int_max_value);
      break;

    case SSET_STRING:
      len = my_snprintf(buf, sizeof(buf),
                        "%-*s %c%c\"%s\"", OPTION_NAME_SPACE, op->name,
                        may_set_option_now(caller, i) ? '+' : ' ',
                        ((strcmp(op->string_value,
                                 op->string_default_value) == 0) ?
                         '=' : ' '), op->string_value);
      break;
    }
    if (len == -1) {
      len = sizeof(buf) - 1;
    }
    /* Line up the descriptions: */
    if (len < len1) {
      cat_snprintf(buf, sizeof(buf), "%*s", (len1 - len), " ");
    } else {
      sz_strlcat(buf, " ");
    }
    sz_strlcat(buf, _(op->short_help));
    cmd_reply_show(buf);
  }
  cmd_reply_show(horiz_line);
  if (level == SSET_VITAL) {
    cmd_reply_show(_("Try 'show situational' or 'show rare' to show "
                     "more options"));
    cmd_reply_show(horiz_line);
  }
  return TRUE;
#undef cmd_reply_show
#undef OPTION_NAME_SPACE
}

/******************************************************************
  Which characters are allowed within option names: (for 'set')
******************************************************************/
static bool is_ok_opt_name_char(char c)
{
  return my_isalnum(c);
}

/******************************************************************
  Which characters are allowed within option values: (for 'set')
******************************************************************/
static bool is_ok_opt_value_char(char c)
{
  return (c == '-') || (c == '*') || (c == '+') || (c == '=')
      || my_isalnum(c);
}

/******************************************************************
  Which characters are allowed between option names and values: (for 'set')
******************************************************************/
static bool is_ok_opt_name_value_sep_char(char c)
{
  return (c == '=') || my_isspace(c);
}

/******************************************************************
...
******************************************************************/
static bool team_command(struct connection *caller, char *str, bool check)
{
  struct player *pplayer;
  enum m_pre_result match_result;
  char buf[MAX_LEN_CONSOLE_LINE];
  char *arg[2];
  int ntokens = 0;
  bool res = FALSE;

  if (server_state != PRE_GAME_STATE || !game.server.is_new_game) {
    cmd_reply(CMD_TEAM, caller, C_SYNTAX,
              _("Cannot change teams once game has begun."));
    return FALSE;
  }

  if (str != NULL || strlen(str) > 0) {
    sz_strlcpy(buf, str);
    ntokens = get_tokens(buf, arg, 2, TOKEN_DELIMITERS);
  }

  if (ntokens > 2 || ntokens < 1) {
    cmd_reply(CMD_TEAM, caller, C_SYNTAX,
              _("Undefined argument.  Usage: team <player> [team]."));
    goto CLEANUP;
  }

  pplayer = find_player_by_name_prefix(arg[0], &match_result);
  if (pplayer == NULL) {
    cmd_reply_no_such_player(CMD_TEAM, caller, arg[0], match_result);
    goto CLEANUP;
  }

  if (!check && pplayer->team != TEAM_NONE) {
    team_remove_player(pplayer);
  }

  if (ntokens == 1) {
    /* Remove from team */
    if (!check) {
      cmd_reply(CMD_TEAM, caller, C_OK, _("Player %s is made teamless."),
                pplayer->name);
    }
    res = TRUE;
    goto CLEANUP;
  }

  if (!is_ascii_name(arg[1])) {
    cmd_reply(CMD_TEAM, caller, C_SYNTAX,
              _("Only ASCII characters are allowed for team names."));
    goto CLEANUP;
  }

  if (is_barbarian(pplayer)) {
    /* This can happen if we change team settings on a loaded game. */
    cmd_reply(CMD_TEAM, caller, C_SYNTAX, _("Cannot team a barbarian."));
    goto CLEANUP;
  }

  if (pplayer->is_observer) {
    /* Not allowed! */
    cmd_reply(CMD_TEAM, caller, C_SYNTAX, _("Cannot team an observer."));
    goto CLEANUP;
  }

  if (!check) {
    team_add_player(pplayer, arg[1]);
    cmd_reply(CMD_TEAM, caller, C_OK, _("Player %s set to team %s."),
              pplayer->name, get_team_name(pplayer->team));
  }
  res = TRUE;

CLEANUP:
  free_tokens(arg, ntokens);
  return res;
}

/******************************************************************
  ...
******************************************************************/
static const char *const vote_args[] = {
  "yes",
  "no",
  "abstain",
  NULL
};
static const char *vote_arg_accessor(int i)
{
  return vote_args[i];
}

/******************************************************************
  Make or participate in a vote.
******************************************************************/
static bool vote_command(struct connection *caller,
                         char *str,
                         bool check)
{
  char buf[MAX_LEN_CONSOLE_LINE];
  char *arg[2];
  int ntokens = 0, i = 0, which = -1;
  enum m_pre_result match_result;
  struct vote *pvote = NULL;
  const char *usage = _("Invalid arguments. Usage: vote "
                        "yes|no|abstain [vote number].");
  bool res = FALSE;

  if (check) {
    /* This should never happen, since /vote must always be
     * set to ALLOW_BASIC or less. But just in case... */
    return FALSE;
  }

  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 2, TOKEN_DELIMITERS);

  if (ntokens == 0) {
    vote_list_iterate(vote_list, pvote) {
      i++;
      cmd_reply(CMD_VOTE, caller, C_COMMENT,
                _("Vote %d \"%s\" (needs %0.0f%%%s%s): %d for, "
                  "%d against, and %d abstained out of %d players."),
                pvote->vote_no, pvote->cmdline,
                pvote->need_pc * 100 + 1,
                pvote->flags & VCF_UNANIMOUS ? _(" unanimous") : "",
                pvote->flags & VCF_NODISSENT ? _(" no dissent") : "",
                pvote->yes, pvote->no, pvote->abstain, game.info.nplayers);
    } vote_list_iterate_end;

    if (i == 0) {
      cmd_reply(CMD_VOTE, caller, C_COMMENT,
                _("There are no votes going on."));
    }

    return TRUE;
  } else if (!connection_can_vote(caller)) {
    cmd_reply(CMD_VOTE, caller, C_FAIL,
              _("You are not allowed to use this command."));
    goto CLEANUP;
  }

  match_result = match_prefix(vote_arg_accessor, VOTE_NUM, 0,
                              mystrncasecmp, arg[0], &i);

  if (match_result == M_PRE_AMBIGUOUS) {
    cmd_reply(CMD_VOTE, caller, C_SYNTAX,
              _("The argument \"%s\" is ambigious."), arg[0]);
    goto CLEANUP;
  } else if (match_result > M_PRE_AMBIGUOUS) {
    /* Failed */
    cmd_reply(CMD_VOTE, caller, C_SYNTAX, usage);
    goto CLEANUP;
  }

  if (ntokens == 1) {
    /* Applies to last vote */
    if (vote_number_sequence > 0 && get_vote_by_no(vote_number_sequence)) {
      which = vote_number_sequence;
    } else {
      int num_votes = vote_list_size(vote_list);
      if (num_votes == 0) {
        cmd_reply(CMD_VOTE, caller, C_FAIL, _("There are no votes running."));
      } else {
        cmd_reply(CMD_VOTE, caller, C_FAIL, _("No legal last vote (%d %s)."),
                  num_votes, PL_("other vote running", "other votes running",
                                 num_votes));
      }
      goto CLEANUP;
    }
  } else {
    if (sscanf(arg[1], "%d", &which) <= 0) {
      cmd_reply(CMD_VOTE, caller, C_SYNTAX, _("Value must be an integer."));
      goto CLEANUP;
    }
  }

  if (!(pvote = get_vote_by_no(which))) {
    cmd_reply(CMD_VOTE, caller, C_FAIL, _("No such vote (%d)."), which);
    goto CLEANUP;
  }

  if (i == VOTE_YES) {
    cmd_reply(CMD_VOTE, caller, C_COMMENT, _("You voted for \"%s\""),
              pvote->cmdline);
    connection_vote(caller, pvote, VOTE_YES);
  } else if (i == VOTE_NO) {
    cmd_reply(CMD_VOTE, caller, C_COMMENT, _("You voted against \"%s\""),
              pvote->cmdline);
    connection_vote(caller, pvote, VOTE_NO);
  } else if (i == VOTE_ABSTAIN) {
    cmd_reply(CMD_VOTE, caller, C_COMMENT,
              _("You abstained from voting on \"%s\""), pvote->cmdline);
    connection_vote(caller, pvote, VOTE_ABSTAIN);
  } else {
    assert(0);                  /* Must never happen */
  }

  res = TRUE;

CLEANUP:
  free_tokens(arg, ntokens);
  return res;
}

/**************************************************************************
  ...
**************************************************************************/
static bool poll_command(struct connection *caller,
                         const char *str,
                         bool check)
{
  char buf[MAX_LEN_CONSOLE_LINE];

  sz_strlcpy(buf, str);
  remove_leading_trailing_spaces(buf);

  if (!caller) {
    cmd_reply(CMD_POLL, caller, C_FAIL,
              _("Only players in the game can make polls."));
    return FALSE;
  }

  if (buf[0] == '\0') {
    cmd_reply(CMD_POLL, caller, C_SYNTAX,
              _("You need to supply a question for the poll!"));
    return FALSE;
  }
  
  /* Do nothing more, the vote system will display the message
   * and show who made the poll. */

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool emote_command(struct connection *caller,
                          const char *str,
                          bool check)
{
  char buf[MAX_LEN_CONSOLE_LINE];
  char chat[MAX_LEN_CONSOLE_LINE];
  char name[MAX_LEN_NAME];

  sz_strlcpy(buf, str);
  remove_leading_trailing_spaces(buf);

  if (buf[0] == '\0') {
    cmd_reply(CMD_EMOTE, caller, C_SYNTAX,
              _("Missing argument."));
    return FALSE;
  }

  if (conn_is_muted(caller)) {
    cmd_reply(CMD_EMOTE, caller, C_REJECTED,
              _("You are not allowed to emote, you are muted!"));
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  if (caller) {
    sz_strlcpy(name, caller->username);
  } else {
    sz_strlcpy(name, "Server console");
  }

  my_snprintf(chat, sizeof(chat), "%c %s %s",
              caller ? '^' : '+', name, buf);

  conn_list_iterate(game.est_connections, dest) {
    if (conn_is_ignored(caller, dest)) {
      continue;
    }
    if (game.server.spectatorchat
        && server_state == RUN_GAME_STATE
        && caller != NULL
        && !connection_controls_player(caller)
        && connection_controls_player(dest)) {
      continue;
    }
    dsend_packet_chat_msg(dest, chat, -1, -1, E_NOEVENT,
                          caller ? caller->id : -1);
  } conn_list_iterate_end;

  return TRUE;
}

/**************************************************************************
  Cancel a vote... /cancelvote <vote number>|all.
**************************************************************************/
static bool cancel_vote_command(struct connection *caller, char *arg,
                                bool check)
{
  struct vote *pvote = NULL;
  int vote_no;

  if (check) {
    /* This should never happen anyway, since /cancelvote
     * is set to ALLOW_BASIC in both pregame and while the
     * game is running. */
    return FALSE;
  }

  remove_leading_trailing_spaces(arg);

  if (arg[0] == '\0') {
    if (caller == NULL) {
      /* Server prompt */
      cmd_reply(CMD_CANCEL_VOTE, caller, C_SYNTAX,
                _("Missing argument <vote number> or "
                  "the string \"all\"."));
      return FALSE;
    }
    if (!(pvote = get_vote_by_caller(caller))) {
      cmd_reply(CMD_CANCEL_VOTE, caller, C_FAIL,
                _("You don't have any vote going on."));
      return FALSE;
    }
  } else if (mystrcasecmp(arg, "all") == 0) {
    if (vote_list_size(vote_list) == 0) {
      cmd_reply(CMD_CANCEL_VOTE, caller, C_FAIL,
                _("There isn't any vote going on."));
      return FALSE;
    } else if (!caller || caller->access_level >= ALLOW_ADMIN) {
      clear_all_votes();
      notify_conn(NULL, _("Server: All votes have been removed."));
      return TRUE;
    } else {
      cmd_reply(CMD_CANCEL_VOTE, caller, C_FAIL,
                _("You are not allowed to use this command."));
      return FALSE;
    }
  } else if (sscanf(arg, "%d", &vote_no) == 1) {
    if (!(pvote = get_vote_by_no(vote_no))) {
      cmd_reply(CMD_CANCEL_VOTE, caller, C_FAIL,
                _("No such vote (%d)."), vote_no);
      return FALSE;
    } else if (caller && caller->access_level < ALLOW_ADMIN
               && caller->id != pvote->caller_id) {
      cmd_reply(CMD_CANCEL_VOTE, caller, C_FAIL,
                _("You are not allowed to cancel this vote (%d)."),
                vote_no);
      return FALSE;
    }
  } else {
    cmd_reply(CMD_CANCEL_VOTE, caller, C_SYNTAX,
              _("Usage: /cancelvote [<vote number>|all]"));
    return FALSE;
  }

  assert(pvote != NULL);

  if (caller) {
    if (caller->id == pvote->caller_id) {
      notify_conn(NULL,
                  _("Server: %s cancelled his vote \"%s\" (number %d)."),
                  caller->username, pvote->cmdline, pvote->vote_no);
    } else {
      notify_conn(NULL,
                  _("Server: %s cancelled the vote \"%s\" (number %d)."),
                  caller->username, pvote->cmdline, pvote->vote_no);
    }
  } else {
    /* Server prompt */
    notify_conn(NULL,
                _
                ("Server: The vote \"%s\" (number %d) has been cancelled."),
                pvote->cmdline, pvote->vote_no);
  }
  /* Make it after, prevent crashs about a free pointer (pvote). */
  remove_vote(pvote);

  return TRUE;
}

/******************************************************************
  ...
******************************************************************/
static bool debug_command(struct connection *caller, char *str, bool check)
{
  char buf[MAX_LEN_CONSOLE_LINE];
  char *arg[3];
  int ntokens = 0, i;
  const char *usage = _("Undefined arguments. Usage: debug <player "
                        "<player> | city <x> <y> | units <x> <y> | "
                        "unit <id>>.");
  if (server_state != RUN_GAME_STATE) {
    cmd_reply(CMD_DEBUG, caller, C_SYNTAX,
              _("Can only use this command once game has begun."));
    return FALSE;
  }
  if (check) {
    return TRUE;                /* whatever! */
  }
  if (str != NULL && strlen(str) > 0) {
    sz_strlcpy(buf, str);
    ntokens = get_tokens(buf, arg, 3, TOKEN_DELIMITERS);
  } else {
    ntokens = 0;
  }
  if (ntokens > 0 && strcmp(arg[0], "player") == 0) {
    struct player *pplayer;
    enum m_pre_result match_result;
    if (ntokens != 2) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, usage);
      goto cleanup;
    }
    pplayer = find_player_by_name_prefix(arg[1], &match_result);
    if (pplayer == NULL) {
      cmd_reply_no_such_player(CMD_DEBUG, caller, arg[1], match_result);
      goto cleanup;
    }
    if (pplayer->debug) {
      pplayer->debug = FALSE;
      cmd_reply(CMD_DEBUG, caller, C_OK, _("%s no longer debugged"),
                pplayer->name);
    } else {
      pplayer->debug = TRUE;
      cmd_reply(CMD_DEBUG, caller, C_OK, _("%s debugged"), pplayer->name);
      /* TODO: print some info about the player here */
    }
  } else if (ntokens > 0 && strcmp(arg[0], "city") == 0) {
    int x, y;
    struct tile *ptile;
    struct city *pcity;
    if (ntokens != 3) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, usage);
      goto cleanup;
    }
    if (sscanf(arg[1], "%d", &x) != 1 || sscanf(arg[2], "%d", &y) != 1) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX,
                _("Value 2 & 3 must be integer."));
      goto cleanup;
    }
    if (!(ptile = map_pos_to_tile(x, y))) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, _("Bad map coordinates."));
      goto cleanup;
    }
    pcity = ptile->city;
    if (!pcity) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX,
                _("No city at this coordinate."));
      goto cleanup;
    }
    if (pcity->debug) {
      pcity->debug = FALSE;
      cmd_reply(CMD_DEBUG, caller, C_OK, _("%s no longer debugged"),
                pcity->name);
    } else {
      pcity->debug = TRUE;
      CITY_LOG(LOG_NORMAL, pcity, "debugged");
      pcity->ai.next_recalc = 0;  /* force recalc of city next turn */
    }
  } else if (ntokens > 0 && strcmp(arg[0], "units") == 0) {
    int x, y;
    struct tile *ptile;
    if (ntokens != 3) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, usage);
      goto cleanup;
    }
    if (sscanf(arg[1], "%d", &x) != 1 || sscanf(arg[2], "%d", &y) != 1) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX,
                _("Value 2 & 3 must be integer."));
      goto cleanup;
    }
    if (!(ptile = map_pos_to_tile(x, y))) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, _("Bad map coordinates."));
      goto cleanup;
    }
    unit_list_iterate(ptile->units, punit) {
      if (punit->debug) {
        punit->debug = FALSE;
        cmd_reply(CMD_DEBUG, caller, C_OK, _("%s's %s no longer debugged."),
                  unit_owner(punit)->name, unit_name(punit->type));
      } else {
        punit->debug = TRUE;
        UNIT_LOG(LOG_NORMAL, punit, _("%s's %s debugged."),
                 unit_owner(punit)->name, unit_name(punit->type));
      }
    } unit_list_iterate_end;
  } else if (ntokens > 0 && strcmp(arg[0], "unit") == 0) {
    int id;
    struct unit *punit;
    if (ntokens != 2) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, usage);
      goto cleanup;
    }
    if (sscanf(arg[1], "%d", &id) != 1) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, _("Value 2 must be integer."));
      goto cleanup;
    }
    if (!(punit = find_unit_by_id(id))) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, _("Unit %d does not exist."),
                id);
      goto cleanup;
    }
    if (punit->debug) {
      punit->debug = FALSE;
      cmd_reply(CMD_DEBUG, caller, C_OK, _("%s's %s no longer debugged."),
                unit_owner(punit)->name, unit_name(punit->type));
    } else {
      punit->debug = TRUE;
      UNIT_LOG(LOG_NORMAL, punit, _("%s's %s debugged."),
               unit_owner(punit)->name, unit_name(punit->type));
    }
  } else {
    cmd_reply(CMD_DEBUG, caller, C_SYNTAX, usage);
  }
cleanup:
  for (i = 0; i < ntokens; i++) {
    free(arg[i]);
  }
  return TRUE;
}

/******************************************************************
  NB Currently used for int options only.
******************************************************************/
static void check_option_capability(struct connection *caller,
                                    struct settings_s *op)
{
  char buf[1024];
  int n = 0;
  struct connection *pconn;

  if (!op->required_capability || !op->required_capability[0])
    return;
  if (*op->int_value == op->int_default_value)
    return;

  my_snprintf(buf, sizeof(buf),
              _("Server: Warning, the setting %s requires the \"%s\" "
                "capability to function correctly when not set to the "
                "default value of %d. The following players do not have "
                "this capability: "),
              op->name, op->required_capability, op->int_default_value);
  /* NB We only check player's capabilities. */
  players_iterate(pplayer) {
    pconn = find_conn_by_user(pplayer->username);
    if (!pconn)
      continue;
    if (has_capability(op->required_capability, pconn->capability)) {
      continue;
    }
    n++;
    cat_snprintf(buf, sizeof(buf), "%s%s", n == 1 ? "" : ", ",
                 pplayer->username);
  } players_iterate_end;
  if (n > 0) {
    notify_conn(game.est_connections, "%s.", buf);
  }
}

/******************************************************************
  ...
******************************************************************/
static bool parse_set_arguments(const char *str, struct setting_value *sv)
{
  const char *cptr_s;
  char *cptr_d;
  char command[MAX_LEN_CONSOLE_LINE], arg[MAX_LEN_CONSOLE_LINE];
  int val;
  struct settings_s *op;

  memset(sv, 0, sizeof(struct setting_value));

  cptr_s = str;
  while (*cptr_s != '\0' && !is_ok_opt_name_char(*cptr_s)) {
    cptr_s++;
  }

  cptr_d = command;
  while (*cptr_s != '\0' && is_ok_opt_name_char(*cptr_s)) {
    *cptr_d++ = *cptr_s++;
  }
  *cptr_d = '\0';

  while (*cptr_s != '\0' && is_ok_opt_name_value_sep_char(*cptr_s)) {
    cptr_s++;
  }

  cptr_d = arg;
  while (*cptr_s != '\0' && is_ok_opt_value_char(*cptr_s)) {
    *cptr_d++ = *cptr_s++;
  }
  *cptr_d = '\0';

  sv->setting_idx = lookup_option(command);

  if (sv->setting_idx < 0) {
    return FALSE;
  }
  op = &settings[sv->setting_idx];

  sz_strlcpy(sv->string_value, arg);
  if (sscanf(arg, "%d", &val) == 1) {
    sv->bool_value = val;
    sv->int_value = val;
  }

  return TRUE;
}

/******************************************************************
  ...
******************************************************************/
static bool set_command(struct connection *caller,
                        struct setting_value *sv, bool check)
{
  int val, cmd;
  struct settings_s *op;
  bool do_update;
  char buffer[500];

  cmd = sv->setting_idx;

  if (cmd == -1 || cmd <= -3) {
    cmd_reply(CMD_SET, caller, C_SYNTAX,
              _("Undefined argument.  Usage: set <option> <value>."));
    return FALSE;
  } else if (cmd == -2) {
    cmd_reply(CMD_SET, caller, C_SYNTAX, _("Ambiguous option name."));
    return FALSE;
  }

  if (!may_set_option(caller, cmd) && !check) {
    cmd_reply(CMD_SET, caller, C_FAIL,
              _("You are not allowed to set this option."));
    return FALSE;
  }

  if (!sset_is_changeable(cmd)) {
    cmd_reply(CMD_SET, caller, C_BOUNCE, _("This setting can't be "
                                           "modified after the game has started."));
    return FALSE;
  }

  op = &settings[cmd];
  do_update = FALSE;
  buffer[0] = '\0';

  /* Prevent shenanigans with the 'rated'
   * setting in running SOLO games. */
  if (server_state == RUN_GAME_STATE
      && op->bool_value == &game.server.rated
      && game.server.fcdb.type == GT_SOLO
      && (game.info.turn >= srvarg.fcdb.min_rated_turns
          || (game.server.rated == FALSE && sv->bool_value == TRUE))) {
    cmd_reply(CMD_SET, caller, C_REJECTED,
              _("Because this is a SOLO game, this setting may "
                "no longer be changed."));
    return FALSE;
  }

  if (map_is_loaded() && op->category == SSET_GEOLOGY) {
    cmd_reply(CMD_SET, caller, C_BOUNCE, _("A fixed Map is loaded, "
                                           "geological settings can't be modified.\n"
                                           "Type /unloadmap in order to unfix mapsettings"));
  } else {
    assert(sv->string_value != NULL);

    switch (op->type) {

    case SSET_BOOL:
      if (sscanf(sv->string_value, "%d", &val) != 1) {
        cmd_reply(CMD_SET, caller, C_SYNTAX,
                  _("Value must be an integer."));
        return FALSE;
      } else if (val != 0 && val != 1) {
        cmd_reply(CMD_SET, caller, C_SYNTAX,
                  _("Value out of range (minimum: 0, maximum: 1)."));
        return FALSE;
      } else {
        const char *reject_message = NULL;
        bool b_val = (val != 0);
        if (settings[cmd].bool_validate
            && !settings[cmd].bool_validate(b_val, &reject_message)) {
          cmd_reply(CMD_SET, caller, C_SYNTAX, reject_message);
          return FALSE;
        } else if (!check) {
          *(op->bool_value) = b_val;
          my_snprintf(buffer, sizeof(buffer),
                      _("Option: %s has been set to %d."), op->name,
                      *(op->bool_value) ? 1 : 0);
          do_update = TRUE;
        }
      }
      break;

    case SSET_INT:
      if (sscanf(sv->string_value, "%d", &val) != 1) {
        cmd_reply(CMD_SET, caller, C_SYNTAX,
                  _("Value must be an integer."));
        return FALSE;
      } else if (val < op->int_min_value || val > op->int_max_value) {
        cmd_reply(CMD_SET, caller, C_SYNTAX,
                  _("Value out of range (minimum: %d, maximum: %d)."),
                  op->int_min_value, op->int_max_value);
        return FALSE;
      } else {
        const char *reject_message = NULL;
        if (settings[cmd].int_validate
            && !settings[cmd].int_validate(val, &reject_message)) {
          cmd_reply(CMD_SET, caller, C_SYNTAX, reject_message);
          return FALSE;
        } else if (!check) {
          *(op->int_value) = val;
          my_snprintf(buffer, sizeof(buffer),
                      _("Option: %s has been set to %d."), op->name,
                      *(op->int_value));
          check_option_capability(caller, op);
          do_update = TRUE;
        }
      }
      break;

    case SSET_STRING:
      if (strlen(sv->string_value) >= op->string_value_size) {
        cmd_reply(CMD_SET, caller, C_SYNTAX, _("String value too "
                                               "long.  Usage: set <option> <value>."));
        return FALSE;
      } else {
        const char *reject_message = NULL;
        if (settings[cmd].string_validate
            && !settings[cmd].string_validate(sv->string_value,
                                              &reject_message)) {
          cmd_reply(CMD_SET, caller, C_SYNTAX, reject_message);
          return FALSE;
        } else if (!check) {
          strcpy(op->string_value, sv->string_value);
          my_snprintf(buffer, sizeof(buffer),
                      _("Option: %s has been set to \"%s\"."),
                      op->name, op->string_value);
          do_update = TRUE;
        }
      }
      break;
    }
  }

  if (!check && strlen(buffer) > 0 && sset_is_to_client(cmd)) {
    notify_conn(NULL, "%s", buffer);
  }

  if (!check && do_update) {
    send_server_info_to_metaserver(META_INFO);

    /* send any modified game parameters to the clients -- if sent
     * before RUN_GAME_STATE, triggers a popdown_races_dialog() call
     * in client/packhand.c#handle_game_info() */

    if (server_state == RUN_GAME_STATE) {
      send_game_info(NULL);
    }
  }

  return TRUE;
}

/**************************************************************************
 check game.server.allow_take for permission to take or observe a player
**************************************************************************/
static bool is_allowed_to_take(struct player *pplayer, bool will_obs,
                               char *msg)
{
  const char *allow;

  if (pplayer->is_civil_war_split) {
    mystrlcpy(msg, _("Sorry, you can't take a player created by a "
                     "civil war."), MAX_LEN_MSG);
    return FALSE;
  }

  if (pplayer->is_observer) {
    if (!
        (allow =
         strchr(game.server.allow_take,
                (game.server.is_new_game ? 'O' : 'o')))) {
      if (will_obs) {
        mystrlcpy(msg, _("Sorry, one can't observe globally in this game."),
                  MAX_LEN_MSG);
      } else {
        mystrlcpy(msg, _("Sorry, you can't take a global observer. Observe "
                         "it instead."), MAX_LEN_MSG);
      }
      return FALSE;
    }
  } else if (is_barbarian(pplayer)) {
    if (!(allow = strchr(game.server.allow_take, 'b'))) {
      if (will_obs) {
        mystrlcpy(msg,
                  _("Sorry, one can't observe barbarians in this game."),
                  MAX_LEN_MSG);
      } else {
        mystrlcpy(msg, _("Sorry, one can't take barbarians in this game."),
                  MAX_LEN_MSG);
      }
      return FALSE;
    }
  } else if (!pplayer->is_alive) {
    if (!(allow = strchr(game.server.allow_take, 'd'))) {
      if (will_obs) {
        mystrlcpy(msg,
                  _("Sorry, one can't observe dead players in this game."),
                  MAX_LEN_MSG);
      } else {
        mystrlcpy(msg,
                  _("Sorry, one can't take dead players in this game."),
                  MAX_LEN_MSG);
      }
      return FALSE;
    }
  } else if (pplayer->ai.control) {
    if (!
        (allow =
         strchr(game.server.allow_take,
                (game.server.is_new_game ? 'A' : 'a')))) {
      if (will_obs) {
        mystrlcpy(msg,
                  _("Sorry, one can't observe AI players in this game."),
                  MAX_LEN_MSG);
      } else {
        mystrlcpy(msg, _("Sorry, one can't take AI players in this game."),
                  MAX_LEN_MSG);
      }
      return FALSE;
    }
  } else {
    if (!
        (allow =
         strchr(game.server.allow_take,
                (game.server.is_new_game ? 'H' : 'h')))) {
      if (will_obs) {
        mystrlcpy(msg,
                  _("Sorry, one can't observe human players in this game."),
                  MAX_LEN_MSG);
      } else {
        mystrlcpy(msg,
                  _("Sorry, one can't take human players in this game."),
                  MAX_LEN_MSG);
      }
      return FALSE;
    }
  }
  allow++;
  if (will_obs && (*allow == '2' || *allow == '3')) {
    mystrlcpy(msg, _("Sorry, one can't observe in this game."),
              MAX_LEN_MSG);
    return FALSE;
  }
  if (!will_obs && *allow == '4') {
    mystrlcpy(msg, _("Sorry, one can't take players in this game."),
              MAX_LEN_MSG);
    return FALSE;
  }
  if (!will_obs && pplayer->is_connected
      && (*allow == '1' || *allow == '3')) {
    mystrlcpy(msg,
              _("Sorry, one can't take players already connected "
                "in this game."), MAX_LEN_MSG);
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
 Observe another player. If we were already attached, detach 
 (see detach_command()). The console and those with ALLOW_HACK can
 use the two-argument command and force others to observe.
**************************************************************************/
static bool observe_command(struct connection *caller, char *str,
                            bool check)
{
  int ntokens = 0;
  char buf[MAX_LEN_CONSOLE_LINE], *arg[2], msg[MAX_LEN_MSG];
  bool is_newgame = (server_state == PRE_GAME_STATE ||
                     server_state == SELECT_RACES_STATE)
      && game.server.is_new_game;
  enum m_pre_result result;
  struct connection *pconn = NULL;
  struct player *pplayer = NULL;
  bool res = FALSE;

  /******** PART I: fill pconn and pplayer ********/
  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 2, TOKEN_DELIMITERS);
  /* check syntax, only certain syntax if allowed depending on the caller */
  if (!caller && ntokens < 1) {
    cmd_reply(CMD_OBSERVE, caller, C_SYNTAX,
              _("Usage: observe [connection-name [player-name]]"));
    goto CLEANUP;
  }

  if (ntokens == 2 && (caller && caller->access_level != ALLOW_HACK)) {
    cmd_reply(CMD_OBSERVE, caller, C_SYNTAX,
              _("Usage: observe [player-name]"));
    goto CLEANUP;
  }
  /* match connection if we're console, match a player if we're not */
  if (ntokens == 1) {
    if (!caller && !(pconn = find_conn_by_user_prefix(arg[0], &result))) {
      cmd_reply_no_such_conn(CMD_OBSERVE, caller, arg[0], result);
      goto CLEANUP;
    } else if (caller
               && !(pplayer =
                    find_player_by_name_prefix(arg[0], &result))) {
      cmd_reply_no_such_player(CMD_OBSERVE, caller, arg[0], result);
      goto CLEANUP;
    }
  }
  /* get connection name then player name */
  if (ntokens == 2) {
    if (!(pconn = find_conn_by_user_prefix(arg[0], &result))) {
      cmd_reply_no_such_conn(CMD_OBSERVE, caller, arg[0], result);
      goto CLEANUP;
    }
    if (!(pplayer = find_player_by_name_prefix(arg[1], &result))) {
      cmd_reply_no_such_player(CMD_OBSERVE, caller, arg[1], result);
      goto CLEANUP;
    }
  }
  /* if we can't force other connections to observe, assign us to be pconn. */
  if (!pconn) {
    pconn = caller;
  }

  /* if we have no pplayer, it means that we want to be a global observer */
  if (!pplayer) {
    const char *allow;

    if (!(allow = strchr(game.server.allow_take,
                         (game.server.is_new_game ? 'O' : 'o')))) {
      cmd_reply(CMD_OBSERVE, caller, C_FAIL,
                _("Sorry, one can't observe globally in this game."));
      goto CLEANUP;
    }
    allow++;

    if (*allow == '2' || *allow == '3') {
      cmd_reply(CMD_OBSERVE, caller, C_FAIL,
                _("Sorry, one can't observe globally in this game."));
      goto CLEANUP;
    }

    /* check if a global observer has already been created */
    players_iterate(aplayer) {
      if (aplayer->is_observer) {
        pplayer = aplayer;
        break;
      }
    } players_iterate_end;

    /* we need to create a new player */
    if (!pplayer) {
      if (game.info.nplayers >= MAX_NUM_PLAYERS) {
        notify_conn(NULL,
                    _("Game: A global observer cannot be created: too "
                      "many regular players."));
        goto CLEANUP;
      }
      pplayer = &game.players[game.info.nplayers];
      server_player_init(pplayer, (server_state == RUN_GAME_STATE)
                         || !game.server.is_new_game);
      sz_strlcpy(pplayer->name, OBSERVER_NAME);
      sz_strlcpy(pplayer->username, ANON_USER_NAME);
      pplayer->is_connected = FALSE;
      pplayer->is_observer = TRUE;
      pplayer->capital = TRUE;
      pplayer->turn_done = TRUE;
      pplayer->embassy = 0;     /* no embassys */
      pplayer->is_alive = FALSE;
      pplayer->was_created = FALSE;
      if ((server_state == RUN_GAME_STATE) || !game.server.is_new_game) {
        pplayer->nation = OBSERVER_NATION;
        init_tech(pplayer);
        give_initial_techs(pplayer);
        map_know_and_see_all(pplayer);
      }

      game.info.nplayers++;

      /* tell everyone that game.info.nplayers has been updated */
      send_game_info(NULL);
      send_player_info(pplayer, NULL);

      notify_conn(NULL, _("Game: A global observer has been created"));
    }
  }

  /******** PART II: do the observing ********/
  /* check allowtake for permission */
  if (!is_allowed_to_take(pplayer, TRUE, msg)) {
    cmd_reply(CMD_OBSERVE, caller, C_FAIL, msg);
    goto CLEANUP;
  }

  /* observing your own player (during pregame) makes no sense. */
  if (pconn->player == pplayer && !pconn->observer
      && is_newgame && !pplayer->was_created) {
    cmd_reply(CMD_OBSERVE, caller, C_FAIL,
              _("%s already controls %s. Using 'observe' would remove %s"),
              pconn->username, pplayer->name, pplayer->name);
    goto CLEANUP;
  }
  /* attempting to observe a player you're already observing should fail. */
  if (pconn->player == pplayer && pconn->observer) {
    cmd_reply(CMD_OBSERVE, caller, C_FAIL,
              _("%s is already observing %s."),
              pconn->username, pplayer->name);
    goto CLEANUP;
  }

  res = TRUE;                   /* all tests passed */
  if (check) {
    goto CLEANUP;
  }

  /* if we want to switch players, reset the client */
  if (pconn->player && server_state == RUN_GAME_STATE) {
    send_game_state(pconn->self, CLIENT_PRE_GAME_STATE);
    send_conn_info(game.est_connections, pconn->self);
  }

  /* if the connection is already attached to a player,
   * unattach and cleanup old player (rename, remove, etc) */
  if (pconn->player) {
    char name[MAX_LEN_NAME];
    /* if a pconn->player is removed, we'll lose pplayer */
    sz_strlcpy(name, pplayer->name);
    detach_command(pconn, "", FALSE);
    /* find pplayer again, the pointer might have been changed */
    pplayer = find_player_by_name(name);
  }
  /* we don't want the connection's username on another player */
  players_iterate(aplayer) {
    if (strncmp(aplayer->username, pconn->username, MAX_LEN_NAME) == 0) {
      sz_strlcpy(aplayer->username, ANON_USER_NAME);
    }
  } players_iterate_end;

  /* attach pconn to new player as an observer */
  pconn->observer = TRUE;       /* do this before attach! */

  attach_connection_to_player(pconn, pplayer);

  send_conn_info(pconn->self, game.est_connections);

  if (server_state >= RUN_GAME_STATE) {
    send_packet_freeze_hint(pconn);
    send_rulesets(pconn->self);
    send_player_info(NULL, NULL);
    send_all_info(pconn->self);
    send_game_state(pconn->self, CLIENT_GAME_RUNNING_STATE);
    send_diplomatic_meetings(pconn);
    send_packet_thaw_hint(pconn);
    send_packet_start_turn(pconn);
    if (server_state == GAME_OVER_STATE) {
      report_final_scores(pconn->self);
      report_game_rankings(pconn->self);
    }
  } else if (server_state == SELECT_RACES_STATE) {
    send_packet_freeze_hint(pconn);
    send_rulesets(pconn->self);
    send_player_info(NULL, NULL);
    send_packet_thaw_hint(pconn);
  }
  cmd_reply(CMD_OBSERVE, caller, C_OK, _("%s now observes %s"),
            pconn->username, pplayer->name);

  send_updated_vote_totals(NULL);

CLEANUP:
  free_tokens(arg, ntokens);
  return res;
}

/**************************************************************************
  Take over a player. If a connection already has control of that player, 
  disallow it. 

  If there are two arguments, treat the first as the connection name and the
  second as the player name (only hack and the console can do this).
  Otherwise, there should be one argument, that being the player that the 
  caller wants to take.
**************************************************************************/
static bool take_command(struct connection *caller, char *str, bool check)
{
  int i = 0, ntokens = 0;
  char buf[MAX_LEN_CONSOLE_LINE], *arg[2], msg[MAX_LEN_MSG];
  bool is_newgame = (server_state == PRE_GAME_STATE ||
                     server_state == SELECT_RACES_STATE)
      && game.server.is_new_game;
  enum m_pre_result match_result;
  struct connection *pconn = NULL;
  struct player *pplayer = NULL;
  bool res = FALSE;

  /******** PART I: fill pconn and pplayer ********/
  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 2, TOKEN_DELIMITERS);
  /* check syntax */
  if (!caller && ntokens != 2) {
    cmd_reply(CMD_TAKE, caller, C_SYNTAX,
              _("Usage: take <connection-name> <player-name>"));
    goto CLEANUP;
  }
  if (caller && caller->access_level != ALLOW_HACK && ntokens != 1) {
    cmd_reply(CMD_TAKE, caller, C_SYNTAX, _("Usage: take <player-name>"));
    goto CLEANUP;
  }
  if (ntokens == 0) {
    cmd_reply(CMD_TAKE, caller, C_SYNTAX,
              _("Usage: take [connection-name] <player-name>"));
    goto CLEANUP;
  }
  if (ntokens == 2) {
    if (!(pconn = find_conn_by_user_prefix(arg[i], &match_result))) {
      cmd_reply_no_such_conn(CMD_TAKE, caller, arg[i], match_result);
      goto CLEANUP;
    }
    i++;                        /* found a conn, now reference the second argument */
  }
  if (!(pplayer = find_player_by_name_prefix(arg[i], &match_result))) {
    cmd_reply_no_such_player(CMD_TAKE, caller, arg[i], match_result);
    goto CLEANUP;
  }

  /* if we don't assign other connections to players, assign us to be pconn. */
  if (!pconn) {
    pconn = caller;
  }

  /******** PART II: do the attaching ********/
  if (!can_control_a_player(pconn, TRUE)) {
    goto CLEANUP;
  }

  /* check allowtake for permission */
  if (!is_allowed_to_take(pplayer, FALSE, msg)) {
    cmd_reply(CMD_TAKE, caller, C_FAIL, msg);
    goto CLEANUP;
  }

  /* you may not take over a global observer */
  if (pplayer->is_observer) {
    cmd_reply(CMD_TAKE, caller, C_FAIL, _("%s cannot be taken."),
              pplayer->name);
    goto CLEANUP;
  }

  /* taking your own player makes no sense. */
  if (pconn->player == pplayer && !pconn->observer) {
    cmd_reply(CMD_TAKE, caller, C_FAIL, _("%s already controls %s"),
              pconn->username, pplayer->name);
    goto CLEANUP;
  }

  res = TRUE;
  if (check) {
    goto CLEANUP;
  }

  /* if we want to switch players, reset the client if the game is running */
  if (pconn->player && server_state == RUN_GAME_STATE) {
    send_game_state(pconn->self, CLIENT_PRE_GAME_STATE);
    send_player_info_c(NULL, pconn->self);
    send_conn_info(game.est_connections, pconn->self);
  }
  /* if we're taking another player with a user attached, 
   * forcibly detach the user from the player. */
  conn_list_iterate(pplayer->connections, aconn) {
    if (!aconn->observer) {
      if (server_state == RUN_GAME_STATE) {
        send_game_state(aconn->self, CLIENT_PRE_GAME_STATE);
      }
      notify_conn(aconn->self, _("being detached from %s."), pplayer->name);
      unattach_connection_from_player(aconn);
      send_conn_info(aconn->self, game.est_connections);
    }
  } conn_list_iterate_end;

  /* if the connection is already attached to a player,
   * unattach and cleanup old player (rename, remove, etc) */
  if (pconn->player) {
    char name[MAX_LEN_NAME];

    /* if a pconn->player is removed, we'll lose pplayer */
    sz_strlcpy(name, pplayer->name);
    detach_command(pconn, "", FALSE);
    /* find pplayer again, the pointer might have been changed */
    pplayer = find_player_by_name(name);
  }

  /* we don't want the connection's username on another player */
  players_iterate(aplayer) {
    if (strncmp(aplayer->username, pconn->username, MAX_LEN_NAME) == 0) {
      sz_strlcpy(aplayer->username, ANON_USER_NAME);
    }
  } players_iterate_end;

  /* now attach to new player */
  attach_connection_to_player(pconn, pplayer);
  send_conn_info(pconn->self, game.est_connections);

  /* if pplayer wasn't /created, and we're still in pregame, change its name */
  if (!pplayer->was_created && is_newgame) {
    sz_strlcpy(pplayer->name, pconn->username);
  }

  if (server_state >= RUN_GAME_STATE) {
    send_packet_freeze_hint(pconn);
    send_rulesets(pconn->self);
    send_all_info(pconn->self);
    send_game_state(pconn->self, CLIENT_GAME_RUNNING_STATE);
    send_player_info(NULL, NULL);
    send_diplomatic_meetings(pconn);
    send_packet_thaw_hint(pconn);
    send_packet_start_turn(pconn);
    if (server_state == GAME_OVER_STATE) {
      report_final_scores(pconn->self);
      report_game_rankings(pconn->self);
    }
  }

  /* aitoggle the player back to human if necessary. */
  if (pplayer->ai.control && game.server.auto_ai_toggle) {
    toggle_ai_player_direct(NULL, pplayer);
  }

  /* yes this has to go after the toggle check */
  if (server_state == RUN_GAME_STATE) {
    gamelog(GAMELOG_PLAYER, pplayer);
  }
  cmd_reply(CMD_TAKE, caller, C_OK, _("%s now controls %s (%s, %s)"),
            pconn->username, pplayer->name,
            is_barbarian(pplayer) ? _("Barbarian") : pplayer->ai.control ?
            _("AI") : _("Human"),
            pplayer->is_alive ? _("Alive") : _("Dead"));

  send_updated_vote_totals(NULL);

CLEANUP:
  free_tokens(arg, ntokens);
  return res;
}

/**************************************************************************
  Detach from a player. if that player wasn't /created and you were 
  controlling the player, remove it (and then detach any observers as well).
**************************************************************************/
static bool detach_command(struct connection *caller, char *str, bool check)
{
  int ntokens = 0;
  char buf[MAX_LEN_CONSOLE_LINE], *arg[1];

  enum m_pre_result match_result;
  struct connection *pconn = NULL;
  struct player *pplayer = NULL;
  bool is_newgame = (server_state == PRE_GAME_STATE ||
                     server_state == SELECT_RACES_STATE)
      && game.server.is_new_game;
  bool one_obs_among_many = FALSE, res = FALSE;
  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 1, TOKEN_DELIMITERS);

  if (!caller && ntokens == 0) {
    cmd_reply(CMD_DETACH, caller, C_SYNTAX,
              _("Usage: detach <connection-name>"));
    goto CLEANUP;
  }
  /* match the connection if the argument was given */
  if (ntokens == 1
      && !(pconn = find_conn_by_user_prefix(arg[0], &match_result))) {
    cmd_reply_no_such_conn(CMD_DETACH, caller, arg[0], match_result);
    goto CLEANUP;
  }

  /* if no argument is given, the caller wants to detach himself */
  if (!pconn) {
    pconn = caller;
  }

  /* if pconn and caller are not the same, only continue 
   * if we're console, or we have ALLOW_HACK */
  if (pconn != caller && caller && caller->access_level != ALLOW_HACK) {
    cmd_reply(CMD_DETACH, caller, C_FAIL,
              _("You can not detach other users."));
    goto CLEANUP;
  }

  pplayer = pconn->player;
  /* must have someone to detach from... */
  if (!pplayer) {
    cmd_reply(CMD_DETACH, caller, C_FAIL,
              _("%s is not attached to any player."), pconn->username);
    goto CLEANUP;
  }

  /* a special case for global observers: we don't want to remove the
   * observer player in pregame if someone else is also observing it */
  if (pplayer->is_observer && conn_list_size(pplayer->connections) > 1) {
    one_obs_among_many = TRUE;
  }

  res = TRUE;
  if (check) {
    goto CLEANUP;
  }

  /* if we want to detach while the game is running, reset the client */
  if (server_state >= RUN_GAME_STATE) {
    send_game_state(pconn->self, CLIENT_PRE_GAME_STATE);
    send_game_info(pconn->self);
    send_player_info_c(NULL, pconn->self);
    send_conn_info(game.est_connections, pconn->self);
  }

  /* actually do the detaching */
  unattach_connection_from_player(pconn);
  send_conn_info(pconn->self, game.est_connections);
  cmd_reply(CMD_DETACH, caller, C_COMMENT,
            _("%s detaching from %s"), pconn->username, pplayer->name);

  /* only remove the player if the game is new and in pregame, 
   * the player wasn't /created, and no one is controlling it 
   * and we were observing but no one else was... */
  if (!pplayer->is_connected && !pplayer->was_created
      && is_newgame && !one_obs_among_many) {
    /* detach any observers */
    conn_list_iterate(pplayer->connections, aconn) {
      if (aconn->observer) {
        unattach_connection_from_player(aconn);
        send_conn_info(aconn->self, game.est_connections);
        notify_conn(aconn->self, _("detaching from %s."), pplayer->name);
      }
    } conn_list_iterate_end;
    /* actually do the removal */
    game_remove_player(pplayer);
    game_renumber_players(pplayer->player_no);
  }

  cancel_connection_votes(pconn);
  send_updated_vote_totals(NULL);

  if (!pplayer->is_connected) {
    /* aitoggle the player if no longer connected. */
    if (game.server.auto_ai_toggle && !pplayer->ai.control) {
      toggle_ai_player_direct(NULL, pplayer);
    }
    /* reset username if in pregame. */
    if (is_newgame) {
      sz_strlcpy(pplayer->username, ANON_USER_NAME);
    }
  }

  if (server_state == RUN_GAME_STATE) {
    gamelog(GAMELOG_PLAYER, pplayer);
  }

CLEANUP:
  free_tokens(arg, ntokens);
  return res;
}

/**************************************************************************
  After a /load is completed, a reply is sent to all connections to tell
  them about the load.  This information is used by the conndlg to
  set up the graphical interface for starting the game.
**************************************************************************/
static void send_load_game_info(bool load_successful)
{
  struct packet_game_load packet;
  /* Clear everything to be safe. */
  memset(&packet, 0, sizeof(packet));
  sz_strlcpy(packet.load_filename, srvarg.load_filename);
  packet.load_successful = load_successful;
  if (load_successful) {
    int i = 0;
    players_iterate(pplayer) {
      if (game.ruleset_control.nation_count && is_barbarian(pplayer)) {
        continue;
      }
      sz_strlcpy(packet.name[i], pplayer->name);
      sz_strlcpy(packet.username[i], pplayer->username);
      if (game.ruleset_control.nation_count) {
        sz_strlcpy(packet.nation_name[i], get_nation_name(pplayer->nation));
        sz_strlcpy(packet.nation_flag[i],
                   get_nation_by_plr(pplayer)->flag_graphic_str);
      } else {                  /* No nations picked */
        sz_strlcpy(packet.nation_name[i], "");
        sz_strlcpy(packet.nation_flag[i], "");
      }
      packet.is_alive[i] = pplayer->is_alive;
      packet.is_ai[i] = pplayer->ai.control;
      i++;
    } players_iterate_end;
    packet.nplayers = i;
  } else {
    packet.nplayers = 0;
  }
  lsend_packet_game_load(game.est_connections, &packet);
}

/**************************************************************************
  ...
**************************************************************************/
static void format_time_duration(time_t t, char *buf, int maxlen)
{
  int seconds, minutes, hours, days;
  bool space = FALSE;

  seconds = t % 60;
  minutes = (t / 60) % 60;
  hours = (t / (60 * 60)) % 24;
  days = t / (60 * 60 * 24);

  if (maxlen <= 0) {
    return;
  }

  buf[0] = '\0';

  if (days > 0) {
    cat_snprintf(buf, maxlen, "%d %s", days, PL_("day", "days", days));
    space = TRUE;
  }
  if (hours > 0) {
    cat_snprintf(buf, maxlen, "%s%d %s",
                 space ? " " : "", hours, PL_("hour", "hours", hours));
    space = TRUE;
  }
  if (minutes > 0) {
    cat_snprintf(buf, maxlen, "%s%d %s",
                 space ? " " : "",
                 minutes, PL_("minute", "minutes", minutes));
    space = TRUE;
  }
  if (seconds > 0) {
    cat_snprintf(buf, maxlen, "%s%d %s",
                 space ? " " : "",
                 seconds, PL_("second", "seconds", seconds));
  }
}

#ifdef HAVE_MYSQL               /* See note for mkstatdate. */
/**************************************************************************
  Returns a value from enum game_types or -1 if there was a parse error.
  On error, send an error message to 'caller' under the command id
  'cmd_id'.
**************************************************************************/
static int parse_game_type(int cmd_id,
                           struct connection *caller, const char *arg)
{
  char buf[128];
  enum game_types type = GT_NUM_TYPES;

  sz_strlcpy(buf, arg);
  remove_leading_trailing_spaces(buf);
  if (buf[0] != '\0') {
    type = game_get_type_from_string(buf);
    if (type == GT_NUM_TYPES) {
      cmd_reply(cmd_id, caller, C_SYNTAX, _("Unrecognized game type."));
      buf[0] = '\0';
      for (type = 0; type < GT_NUM_TYPES; type++) {
        cat_snprintf(buf, sizeof(buf), " %s", game_type_name(type));
      }
      cmd_reply(cmd_id, caller, C_COMMENT,
                _("The valid game types are:%s"), buf);
      return -1;
    }
  }
  return type;
}

/**************************************************************************
  ...
**************************************************************************/
struct rated_player
{
  const char *player_name;
  char username[256];
  double rating, rd;
};

/**************************************************************************
  ...
**************************************************************************/
static int rate_compare(const void *a, const void *b)
{
  double rating1, rating2, rd1, rd2;

  rating1 = ((const struct rated_player *) a)->rating;
  rating2 = ((const struct rated_player *) b)->rating;
  rd1 = ((const struct rated_player *) a)->rd;
  rd2 = ((const struct rated_player *) b)->rd;

  /* To avoid any complications due to double-to-int
   * implicit conversion, compare the double values
   * directly. */
  if (rating1 > rating2) {
    return -1;
  } else if (rating1 < rating2) {
    return 1;
  } else if (rd1 > rd2) {
    return 1;
  } else if (rd1 < rd2) {
    return -1;
  }

  return 0;
}

/**************************************************************************
  ...
**************************************************************************/
static bool ratings_command(struct connection *caller,
                            char *arg, bool check)
{
  char buf[128], fmt[128], fmt2[128];
  int i, cml_pname, cml_uname;
  int type = GT_NUM_TYPES;
  struct rated_player list[game.info.nplayers];

  if (!srvarg.fcdb.enabled) {
    cmd_reply(CMD_RATINGS, caller, C_GENFAIL,
              _("This server does not have database support enabled."));
    return FALSE;
  }

  if (game.info.nplayers <= 0) {
    cmd_reply(CMD_RATINGS, caller, C_FAIL,
              _("There are no players in the game."));
    return FALSE;
  }

  if ((type = parse_game_type(CMD_RATINGS, caller, arg)) == -1) {
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  if (type == GT_NUM_TYPES) {
    type = game_determine_type();
  }

  sz_strlcpy(buf, game_type_name(type));
  for (i = 0; buf[i] != '\0'; i++) {
    buf[i] = my_toupper(buf[i]);
  }

  i = 0;
  cml_pname = 8;
  cml_uname = 10;

  players_iterate(pplayer) {
    if (pplayer->is_observer || is_barbarian(pplayer)
        || pplayer->is_civil_war_split) {
      continue;
    }

    list[i].player_name = pplayer->name;
    list[i].rating = 0;
    list[i].rd = 0;

    cml_pname = MAX(cml_pname, strlen(pplayer->name) + 2);

    if (pplayer->fcdb.rated_user_name[0] != '\0') {
      if (!fcdb_get_user_rating(pplayer->fcdb.rated_user_name,
                                type, &list[i].rating, &list[i].rd)) {
        cmd_reply(CMD_RATINGS, caller, C_GENFAIL,
                  _("An error occurred while reading from the database."));
        return FALSE;
      }
    } else if (pplayer->username[0] != '\0'
               && 0 != strcmp(pplayer->username, ANON_USER_NAME)) {
      if (!fcdb_get_user_rating(pplayer->username, type,
                                &list[i].rating, &list[i].rd)) {
        cmd_reply(CMD_RATINGS, caller, C_GENFAIL,
                  _("An error occurred while reading from the database."));
        return FALSE;
      }
    } else {
      /* Assume it is an AI... */
      score_get_ai_rating(pplayer->ai.skill_level, type,
                          &list[i].rating, &list[i].rd);
    }

    player_get_rated_username(pplayer, list[i].username,
                              sizeof(list[i].username));
    cml_uname = MAX(cml_uname, strlen(list[i].username) + 2);
    i++;
  } players_iterate_end;

  qsort(list, game.info.nplayers, sizeof(struct rated_player),
        rate_compare);

  cmd_reply(CMD_RATINGS, caller, C_COMMENT, _("%s Ratings"), buf);
  cmd_reply(CMD_RATINGS, caller, C_COMMENT, horiz_line);

  my_snprintf(fmt, sizeof(fmt), "%%-%ds %%-%ds %%10s %%10s",
              cml_pname, cml_uname);
  cmd_reply(CMD_RATINGS, caller, C_COMMENT, fmt,
            _("Player"), _("Username"), _("Rating"), _("RD"));

  my_snprintf(fmt2, sizeof(fmt2), "%%-%ds %%-%ds %%10.2f %%10.2f",
              cml_pname, cml_uname);

  for (i = 0; i < game.info.nplayers; i++) {
    if (list[i].rating > 0.0) {
      cmd_reply(CMD_RATINGS, caller, C_COMMENT, fmt2,
                list[i].player_name, list[i].username,
                list[i].rating, list[i].rd);
    } else {
      cmd_reply(CMD_RATINGS, caller, C_COMMENT, fmt,
                list[i].player_name, list[i].username,
                _("<unrated>"), _("<unrated>"));
    }
  }
  cmd_reply(CMD_RATINGS, caller, C_COMMENT, horiz_line);

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool examine_command(struct connection *caller,
                            char *arg, bool check)
{
  int id, i, num_rating_changes;
  char buf[256], fmt[128];
  struct fcdb_game_info *fgi;
  struct fcdb_player_in_game_info *fpi;
  struct fcdb_team_in_game_info *fti;

  if (!srvarg.fcdb.enabled) {
    cmd_reply(CMD_RATINGS, caller, C_GENFAIL,
              _("This server does not have database support enabled."));
    return FALSE;
  }

  sz_strlcpy(buf, arg);
  remove_leading_trailing_spaces(buf);
  id = atoi(buf);

  if (id <= 0) {
    cmd_reply(CMD_EXAMINE, caller, C_SYNTAX,
              _("The game number must be a positive integer."));
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  if (!(fgi = fcdb_game_info_new(id))) {
    cmd_reply(CMD_EXAMINE, caller, C_GENFAIL,
              _("There was an error reading from the database."));
    return FALSE;
  }

  if (fgi->id <= 0) {
    cmd_reply(CMD_EXAMINE, caller, C_FAIL, _("No such game: %d."), id);
    fcdb_game_info_free(fgi);
    return FALSE;
  }

  if (!fgi->completed && !(caller == NULL
                           || caller->access_level >= ALLOW_ADMIN)) {
    cmd_reply(CMD_EXAMINE, caller, C_REJECTED,
              _("You may not examine game #%d."), id);
    return FALSE;
  }

  cmd_reply(CMD_EXAMINE, caller, C_COMMENT, "%s", horiz_line);
  cmd_reply(CMD_EXAMINE, caller, C_COMMENT,
            _("Information For Game #%d"), fgi->id);
  cmd_reply(CMD_EXAMINE, caller, C_COMMENT, "%s", horiz_line);
  cmd_reply(CMD_EXAMINE, caller, C_COMMENT,
            _("  Host: %-16s Port: %d"), fgi->host, fgi->port);

  mkstatdate(fgi->created, buf, sizeof(buf));
  cmd_reply(CMD_EXAMINE, caller, C_COMMENT, _("  Created: %s"), buf);

  format_time_duration(fgi->duration, buf, sizeof(buf));
  cmd_reply(CMD_EXAMINE, caller, C_COMMENT, _("  Duration: %s"), buf);

  cmd_reply(CMD_EXAMINE, caller, C_COMMENT,
            _("  Type: %-8s Turns: %-8d Outcome: %s"),
            fgi->type, fgi->num_turns, fgi->outcome);

  if (fgi->num_teams > 0) {
    int colmaxlen = 6;
    
    /* Pre-calculate the maximum required width
     * of the 'Name' column. */
    for (i = 0; i < fgi->num_teams; i++) {
      fti = &fgi->teams[i];
      colmaxlen = MAX(colmaxlen, strlen(fti->name) + 1);
    }

    cmd_reply(CMD_EXAMINE, caller, C_COMMENT, "%s", horiz_line);
    cmd_reply(CMD_EXAMINE, caller, C_COMMENT,
              _("  Teams (%d):"), fgi->num_teams);

    my_snprintf(fmt, sizeof(fmt), "    %%-%ds %%7s %%8s %%8s",
                colmaxlen);
    cmd_reply(CMD_EXAMINE, caller, C_COMMENT, fmt,
              _("Name"), _("Rank"), _("Score"), _("Result"));

    my_snprintf(fmt, sizeof(fmt), "    %%-%ds %%7.1f %%8.1f %%8s",
                colmaxlen);
    for (i = 0; i < fgi->num_teams; i++) {
      fti = &fgi->teams[i];
      cmd_reply(CMD_EXAMINE, caller, C_COMMENT, fmt,
                fti->name, fti->rank + 1.0, fti->score, fti->result);
    }
  }

  if (fgi->num_players > 0) {
    int cml_name = 6;
    int cml_user = 6;
    int cml_nation = 8;
    int cml_team = 6;

    for (i = 0; i < fgi->num_players; i++) {
      fpi = &fgi->players[i];
      cml_name = MAX(cml_name, strlen(fpi->name) + 1);
      cml_user = MAX(cml_user, strlen(fpi->user) + 1);
      cml_nation = MAX(cml_nation, strlen(fpi->nation) + 1);
      cml_team = MAX(cml_team, strlen(fpi->team_name) + 1);
    }

    cmd_reply(CMD_EXAMINE, caller, C_COMMENT, "%s", horiz_line);
    cmd_reply(CMD_EXAMINE, caller, C_COMMENT,
              _("  Players (%d):"), fgi->num_players);

    my_snprintf(fmt, sizeof(fmt),
                "    %%-%ds %%-%ds %%-%ds %%-%ds %%7s %%8s %%8s",
                cml_name, cml_user, cml_nation, cml_team);
    cmd_reply(CMD_EXAMINE, caller, C_COMMENT, fmt,
              _("Name"), _("User"), _("Nation"), _("Team"),
              _("Rank"), _("Score"), _("Result"));

    my_snprintf(fmt, sizeof(fmt),
                "    %%-%ds %%-%ds %%-%ds %%-%ds %%7.1f %%8.1f %%8s",
                cml_name, cml_user, cml_nation, cml_team);

    for (i = 0; i < fgi->num_players; i++) {
      fpi = &fgi->players[i];
      if (fpi->team_name == NULL || fpi->team_name[0] == '\0') {
        my_snprintf(buf, sizeof(buf), "<none>");
      } else {
        sz_strlcpy(buf, fpi->team_name);
      }
      cmd_reply(CMD_EXAMINE, caller, C_COMMENT, fmt,
                fpi->name, fpi->user, fpi->nation, buf,
                fpi->rank + 1.0, fpi->score, fpi->result);
    }

    cml_user = 6;
    num_rating_changes = 0;
    for (i = 0; i < fgi->num_players; i++) {
      fpi = &fgi->players[i];
      if (fpi->user[0] == '\0' || fpi->old_rating == 0.0
          || fpi->new_rating == 0.0) {
        continue;
      }
      num_rating_changes++;
      cml_user = MAX(cml_user, strlen(fpi->user) + 1);
    }

    if (num_rating_changes > 0) {
      cmd_reply(CMD_EXAMINE, caller, C_COMMENT, "%s", horiz_line);
      cmd_reply(CMD_EXAMINE, caller, C_COMMENT, "%s",
                _("  Rating Changes:"));


      my_snprintf(fmt, sizeof(fmt),
                  "    %%-%ds %%12s %%12s %%12s %%12s",
                  cml_user);
      cmd_reply(CMD_EXAMINE, caller, C_COMMENT, fmt,
                _("User"), _("Old Rating"), _("Old RD"),
                _("New Rating"), _("New RD"));

      for (i = 0; i < fgi->num_players; i++) {
        fpi = &fgi->players[i];
        if (fpi->user[0] == '\0' || fpi->old_rating == 0.0
            || fpi->new_rating == 0.0) {
          continue;
        }
        my_snprintf(fmt, sizeof(fmt),
                    "    %%-%ds %%12.2f %%12.2f %%12.2f %%12.2f",
                    cml_user);
        cmd_reply(CMD_EXAMINE, caller, C_COMMENT, fmt,
                  fpi->user, fpi->old_rating, fpi->old_rd,
                  fpi->new_rating, fpi->new_rd);
      }
    }
  }

  cmd_reply(CMD_EXAMINE, caller, C_COMMENT, "%s", horiz_line);


  fcdb_game_info_free(fgi);
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool topten_command(struct connection *caller, char *arg, bool check)
{
  int type = GT_NUM_TYPES;
  struct fcdb_topten_info *ftti = NULL;
  struct fcdb_topten_info_entry *tte = NULL;
  char buf[64], fmt[128];
  int i, cml_name;

  if (!srvarg.fcdb.enabled) {
    cmd_reply(CMD_TOPTEN, caller, C_GENFAIL,
              _("This server does not have database support enabled."));
    return FALSE;
  }

  if ((type = parse_game_type(CMD_TOPTEN, caller, arg)) == -1) {
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  if (type == GT_NUM_TYPES) {
    type = game_determine_type();
  }

  if (!(ftti = fcdb_topten_info_new(type))) {
    cmd_reply(CMD_TOPTEN, caller, C_GENFAIL,
              _("There was an error reading from the database."));
    return FALSE;
  }

  sz_strlcpy(buf, game_type_name(type));
  for (i = 0; buf[i] != '\0'; i++) {
    buf[i] = my_toupper(buf[i]);
  }

  if (ftti->count <= 0) {
    cmd_reply(CMD_TOPTEN, caller, C_FAIL,
              _("There are no rated players for the %s game type."), buf);
    fcdb_topten_info_free(ftti);
    return TRUE;
  }

  cml_name = 6;
  for (i = 0; i < ftti->count; i++) {
    tte = &ftti->entries[i];
    cml_name = MAX(cml_name, strlen(tte->user) + 2);
  }

  cmd_reply(CMD_TOPTEN, caller, C_COMMENT, horiz_line);
  cmd_reply(CMD_TOPTEN, caller, C_COMMENT, _("Top %s Players"), buf);
  cmd_reply(CMD_TOPTEN, caller, C_COMMENT, horiz_line);

  my_snprintf(fmt, sizeof(fmt), "%%4s %%-%ds %%10s %%8s %%18s",
              cml_name);
  cmd_reply(CMD_TOPTEN, caller, C_COMMENT, fmt, _("Rank"), _("User"),
            _("Rating"), _("RD"), _("Wins/Loses/Draws"));

  my_snprintf(fmt, sizeof(fmt), "%%4d %%-%ds %%10.2f %%8.2f %%18s",
              cml_name);

  for (i = 0; i < ftti->count; i++) {
    tte = &ftti->entries[i];
    my_snprintf(buf, sizeof(buf), "%d/%d/%d",
                tte->wins, tte->loses, tte->draws);
    cmd_reply(CMD_TOPTEN, caller, C_COMMENT, fmt,
              i + 1, tte->user, tte->rating, tte->rd, buf);
  }
  cmd_reply(CMD_TOPTEN, caller, C_COMMENT, horiz_line);

  fcdb_topten_info_free(ftti);

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool gamelist_command(struct connection *caller,
                             char *arg, bool check)
{
  struct fcdb_gamelist *fgl = NULL;
  struct fcdb_gamelist_entry *fgle = NULL;
  char buf[128];
  char user[MAX_LEN_NAME] = "";
  int type = GT_NUM_TYPES, i;
  int ntokens;
  int first = 0, last = 0;
  char *args[2];

  if (!srvarg.fcdb.enabled) {
    cmd_reply(CMD_GAMELIST, caller, C_GENFAIL,
              _("This server does not have database support enabled."));
    return FALSE;
  }

  ntokens = get_tokens(arg, args, 2, " =\n\t");

  if (ntokens == 2) {
    if (0 == strcmp(args[0], "user")) {
      sz_strlcpy(user, args[1]);
    } else if (0 == strcmp(args[0], "type")) {
      if (-1 == (type = parse_game_type(CMD_GAMELIST, caller, args[1]))) {
        free_tokens(args, ntokens);
        return FALSE;
      }
    }
  } else if (ntokens == 1) {
    char err[256];
    bool ok;

    ok = parse_range(args[0], &first, &last, err, sizeof(err));

    freelog(LOG_DEBUG, "gamelist parsed range %d %d", first, last);

    if (!ok) {
      cmd_reply(CMD_GAMELIST, caller, C_SYNTAX,
                _("Invalid range: %s."), err);
      free_tokens(args, ntokens);
      return FALSE;
    }
  }

  free_tokens(args, ntokens);

  if (check) {
    return TRUE;
  }

  if (!(fgl = fcdb_gamelist_new(type, user, first, last))) {
    cmd_reply(CMD_GAMELIST, caller, C_GENFAIL,
              _("There was an error reading from the database."));
    return FALSE;
  }

  if (user[0] != '\0' && fgl->id <= 0) {
    cmd_reply(CMD_GAMELIST, caller, C_FAIL,
              _("No such user \"%s\"."), user);
    fcdb_gamelist_free(fgl);
    return FALSE;
  }

  if (fgl->count <= 0) {
    cmd_reply(CMD_GAMELIST, caller, C_FAIL,
              _("No matching games to list."));
    fcdb_gamelist_free(fgl);
    return FALSE;
  }

  cmd_reply(CMD_GAMELIST, caller, C_COMMENT, "%7s %10s %8s %24s  %s",
            _("Game #"), _("Type"), _("Players"), _("Date"), _("Outcome"));
  for (i = 0; i < fgl->count; i++) {
    fgle = &fgl->entries[i];
    mkstatdate(fgle->created, buf, sizeof(buf));
    cmd_reply(CMD_GAMELIST, caller, C_COMMENT, "%7d %10s %8d %24s  %s",
              fgle->id, fgle->type, fgle->players, buf, fgle->outcome);
  }

  fcdb_gamelist_free(fgl);

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool aka_command(struct connection *caller,
                        const char *arg, bool check)
{
  struct fcdb_aliaslist *fal = NULL;
  struct fcdb_aliaslist_entry *fale = NULL;
  char buf[MAX_LEN_MSG];
  char namebuf[MAX_LEN_NAME];
  char user[MAX_LEN_NAME];
  int i, nb, len;

  if (!srvarg.fcdb.enabled) {
    cmd_reply(CMD_AKA, caller, C_GENFAIL,
              _("This server does not have database support enabled."));
    return FALSE;
  }

  sz_strlcpy(buf, arg);
  remove_leading_trailing_spaces(buf);
  sz_strlcpy(user, buf);

  if (user[0] == '\0') {
    cmd_reply(CMD_AKA, caller, C_SYNTAX, _("Missing user name argument."));
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  if (!(fal = fcdb_aliaslist_new(user))) {
    cmd_reply(CMD_AKA, caller, C_GENFAIL,
              _("There was an error reading from the database."));
    return FALSE;
  }

  if (fal->id <= 0) {
    cmd_reply(CMD_AKA, caller, C_FAIL,
              _("No such user \"%s\" in the database."), user);
    fcdb_aliaslist_free(fal);
    return FALSE;
  }

  if (fal->count < 1) {
    cmd_reply(CMD_AKA, caller, C_COMMENT,
              _("User %s has no aliases."), user);
    fcdb_aliaslist_free(fal);
    return TRUE;
  }

  nb = my_snprintf(buf, sizeof(buf), _("Aliases for user %s:"), user);
  for (i = 0; i < fal->count; i++) {
    fale = &fal->entries[i];
    if (fale->name[0] == '\0') {
      len = my_snprintf(namebuf, sizeof(namebuf), " user#%d", fale->id);
    } else {
      len = my_snprintf(namebuf, sizeof(namebuf), " <%s>", fale->name);
    }
    if (len + nb >= MAX_LEN_MSG - 1) {
      cmd_reply(CMD_AKA, caller, C_COMMENT, "%s", buf);
      nb = my_snprintf(buf, sizeof(buf),
                       _("More aliases for user %s:"), user);
    }
    nb = cat_snprintf(buf, sizeof(buf), "%s", namebuf);
  }
  cmd_reply(CMD_AKA, caller, C_COMMENT, "%s", buf);

  fcdb_aliaslist_free(fal);

  return TRUE;
}

/**************************************************************************
  Helpers for fcdb_command.
**************************************************************************/
enum fcdb_args
{
  FCDB_ON, FCDB_OFF, FCDB_MIN_RATED_TURNS, FCDB_SAVE_MAPS, FCDB_NUM_ARGS
};
static const char *const fcdb_args[] = {
  "on", "off", "min_rated_turns", "save_maps", NULL
};
static const char *fcdbarg_accessor(int i)
{
  return fcdb_args[i];
}

/**************************************************************************
  ...
**************************************************************************/
static bool fcdb_command(struct connection *caller, char *arg, bool check)
{
  int ind, n, ntokens;
  enum m_pre_result match_result;
  char *args[2];

  /* Only server operator can use this anyway. */
  if (check) {
    return TRUE;
  }

  ntokens = get_tokens(arg, args, 2, TOKEN_DELIMITERS);

  if (ntokens <= 0) {
    cmd_reply(CMD_FCDB, caller, C_COMMENT,
              _("FCDB access: %s."),
              srvarg.fcdb.enabled ? _("enabled") : _("disabled"));
    cmd_reply(CMD_FCDB, caller, C_COMMENT,
              _("Minimum turns for rated game: %d."),
              srvarg.fcdb.min_rated_turns);
    cmd_reply(CMD_FCDB, caller, C_COMMENT,
              _("Saving of encoded turn maps: %s."),
              srvarg.fcdb.save_maps ? _("enabled") : _("disabled"));
    free_tokens(args, ntokens);
    return TRUE;
  }

  match_result = match_prefix(fcdbarg_accessor, FCDB_NUM_ARGS, 0,
                              mystrncasecmp, args[0], &ind);

  if (match_result > M_PRE_EMPTY) {
    cmd_reply(CMD_FCDB, caller, C_SYNTAX,
              _("Unrecognized argument: \"%s\"."), args[0]);
    free_tokens(args, ntokens);
    return FALSE;
  }

  if (ind == FCDB_ON) {
    srvarg.fcdb.enabled = TRUE;
    cmd_reply(CMD_FCDB, caller, C_OK, _("FCDB access enabled."));

  } else if (ind == FCDB_OFF) {
    srvarg.fcdb.enabled = FALSE;
    cmd_reply(CMD_FCDB, caller, C_OK, _("FCDB access disabled."));

  } else if (ind == FCDB_MIN_RATED_TURNS) {
    if (ntokens < 2) {
      cmd_reply(CMD_FCDB, caller, C_SYNTAX, _("Number argument missing."));
      free_tokens(args, ntokens);
      return FALSE;
    }

    n = atoi(args[1]);
    if (n < 0 || (n == 0 && !(args[1][0] == '0' && args[1][1] == '\0'))) {
      cmd_reply(CMD_FCDB, caller, C_SYNTAX,
                _("The number argument must be a non-negative integer."));
      free_tokens(args, ntokens);
      return FALSE;
    }

    srvarg.fcdb.min_rated_turns = n;
    cmd_reply(CMD_FCDB, caller, C_OK,
              _("The minimum number of turns for a rated game has "
                "been set to %d."), n);
  } else if (ind == FCDB_SAVE_MAPS) {
    if (ntokens < 2) {
      cmd_reply(CMD_FCDB, caller, C_SYNTAX,
                _("Missing 'on' or 'off' argument."));
      free_tokens(args, ntokens);
      return FALSE;
    }
    if (0 == strcmp(args[1], "yes") || 0 == strcmp(args[1], "y")
        || 0 == strcmp(args[1], "on")) {
      srvarg.fcdb.save_maps = TRUE;
    } else if (0 == strcmp(args[1], "no") || 0 == strcmp(args[1], "n")
        || 0 == strcmp(args[1], "off")) {
      srvarg.fcdb.save_maps = FALSE;
    } else {
      cmd_reply(CMD_FCDB, caller, C_SYNTAX,
                _("Invalid argument."));
      free_tokens(args, ntokens);
      return FALSE;
    }
    cmd_reply(CMD_FCDB, caller, C_COMMENT,
              _("Saving of encoded turn maps: %s."),
              srvarg.fcdb.save_maps ? _("enabled") : _("disabled"));
  }

  free_tokens(args, ntokens);
  return TRUE;
}

/**************************************************************************
  'authdb' arguments
**************************************************************************/
enum AUTHDB_ARGS
{
  AUTHDB_ARG_HOST,
  AUTHDB_ARG_USER,
  AUTHDB_ARG_PASSWORD,
  AUTHDB_ARG_DATABASE,
  AUTHDB_ARG_ON,
  AUTHDB_ARG_OFF,
  AUTHDB_ARG_GUESTS,
  AUTHDB_ARG_NEWUSERS,
  AUTHDB_NUM_ARGS,
};
static const char *const authdb_args[] = {
  "host", "user", "password", "database", "on", "off", "guests",
  "newusers", NULL
};
static const char *authdbarg_accessor(int i)
{
  return authdb_args[i];
}

/**************************************************************************
  Control password database parameters.
**************************************************************************/
static bool authdb_command(struct connection *caller, char *arg, bool check)
{
  int ind;
  enum m_pre_result match_result;
  char *lastarg = NULL, *p;
  if (check)
    return TRUE;
  remove_leading_trailing_spaces(arg);
  if ((p = strchr(arg, ' '))) {
    *p++ = 0;
    lastarg = p;
  }
  remove_leading_trailing_spaces(arg);
  match_result = match_prefix(authdbarg_accessor, AUTHDB_NUM_ARGS, 0,
                              mystrncasecmp, arg, &ind);
  if (match_result > M_PRE_EMPTY) {
    cmd_reply(CMD_AUTHDB, caller, C_SYNTAX,
              _("Unrecognized argument: \"%s\"."), arg);
    return FALSE;
  }
  if (match_result == M_PRE_EMPTY) {
    cmd_reply(CMD_AUTHDB, caller, C_COMMENT,
              _("Authorization Database Parameters (%s):"),
              srvarg.auth.enabled ? _("enabled") : _("disabled"));
    cmd_reply(CMD_AUTHDB, caller, C_COMMENT, horiz_line);
    cmd_reply(CMD_AUTHDB, caller, C_COMMENT,
              _("Host: \"%s\"\nUser: \"%s\"\nPassword: \"%s\"\n"
                "Database: \"%s\"\nGuests: %s\nNew users: %s"),
              fcdb.host, fcdb.user, fcdb.password,
              fcdb.dbname,
              srvarg.auth.allow_guests ? "allowed" : "not allowed",
              srvarg.auth.allow_newusers ? "allowed" : "not allowed");
    cmd_reply(CMD_AUTHDB, caller, C_COMMENT, horiz_line);
    return TRUE;
  }
  if (ind == AUTHDB_ARG_ON) {
    srvarg.auth.enabled = TRUE;
    cmd_reply(CMD_AUTHDB, caller, C_COMMENT, "Authorization enabled.");
    return TRUE;
  } else if (ind == AUTHDB_ARG_OFF) {
    srvarg.auth.enabled = FALSE;
    cmd_reply(CMD_AUTHDB, caller, C_COMMENT, "Authorization disabled.");
    return TRUE;
  }
  if (!lastarg) {
    cmd_reply(CMD_AUTHDB, caller, C_SYNTAX,
              _("Required argument missing."));
    return FALSE;
  }
  remove_leading_trailing_spaces(lastarg);
  switch (ind) {
  case AUTHDB_ARG_HOST:
    sz_strlcpy(fcdb.host, lastarg);
    break;
  case AUTHDB_ARG_PASSWORD:
    sz_strlcpy(fcdb.password, lastarg);
    break;
  case AUTHDB_ARG_DATABASE:
    sz_strlcpy(fcdb.dbname, lastarg);
    break;
  case AUTHDB_ARG_USER:
    sz_strlcpy(fcdb.user, lastarg);
    break;
  case AUTHDB_ARG_GUESTS:
    if (!strcmp(lastarg, "yes")) {
      srvarg.auth.allow_guests = TRUE;
      cmd_reply(CMD_AUTHDB, caller, C_COMMENT,
                _("Guests are now allowed."));
      return TRUE;
    } else if (!strcmp(lastarg, "no")) {
      srvarg.auth.allow_guests = FALSE;
      cmd_reply(CMD_AUTHDB, caller, C_COMMENT,
                _("Guests are now not allowed."));
      return TRUE;
    } else {
      cmd_reply(CMD_AUTHDB, caller, C_SYNTAX,
                _("The only valid argument in \"yes\" or \"no\"."));
      return FALSE;
    }
    break;
  case AUTHDB_ARG_NEWUSERS:
    if (!strcmp(lastarg, "yes")) {
      srvarg.auth.allow_newusers = TRUE;
      cmd_reply(CMD_AUTHDB, caller, C_COMMENT,
                _("New users are now allowed."));
      return TRUE;
    } else if (!strcmp(lastarg, "no")) {
      srvarg.auth.allow_newusers = FALSE;
      cmd_reply(CMD_AUTHDB, caller, C_COMMENT,
                _("New users are now not allowed."));
      return TRUE;
    } else {
      cmd_reply(CMD_AUTHDB, caller, C_SYNTAX,
                _("The only valid argument in \"yes\" or \"no\"."));
      return FALSE;
    }
    break;
  default:
    break;
  }
  cmd_reply(CMD_AUTHDB, caller, C_COMMENT,
            _("AUTH %s set to \"%s\"."), authdb_args[ind], lastarg);
  return TRUE;
}
#endif /* HAVE_MYSQL */
/**************************************************************************
  ...
**************************************************************************/
bool load_command(struct connection * caller, char *filename, bool check)
{
  struct timer *loadtimer, *uloadtimer;
  struct section_file file;
  char arg[strlen(filename) + 1];

  /* We make a local copy because the parameter might be a pointer to 
   * srvarg.load_filename, which we edit down below. */
  sz_strlcpy(arg, filename);

  if (arg[0] == '\0') {
    cmd_reply(CMD_LOAD, caller, C_FAIL, _("Usage: load <filename>"));
    send_load_game_info(FALSE);
    return FALSE;
  }

  if (server_state != PRE_GAME_STATE) {
    cmd_reply(CMD_LOAD, caller, C_FAIL, _("Can't load a game while another "
                                          "is running."));
    send_load_game_info(FALSE);
    return FALSE;
  }
  /* attempt to parse the file */
  if (!section_file_load_nodup(&file, arg)) {
    cmd_reply(CMD_LOAD, caller, C_FAIL, _("Couldn't load savefile: %s"),
              arg);
    send_load_game_info(FALSE);
    return FALSE;
  }
  if (check) {
    return TRUE;
  }

  /* we found it, free all structures */
  server_game_free();
  server_game_init();

  loadtimer = new_timer_start(TIMER_CPU, TIMER_ACTIVE);
  uloadtimer = new_timer_start(TIMER_USER, TIMER_ACTIVE);

  sz_strlcpy(srvarg.load_filename, arg);
  game_load(&file);
  section_file_check_unused(&file, arg);
  section_file_free(&file);

  freelog(LOG_VERBOSE, "Load time: %g seconds (%g apparent)",
          read_timer_seconds(loadtimer), read_timer_seconds(uloadtimer));

  free_timer(loadtimer);
  free_timer(uloadtimer);

  sanity_check();

  /* Everything seemed to load ok; spread the good news. */
  send_load_game_info(TRUE);

  /* attach connections to players. currently, this applies only 
   * to connections that have the correct username. */
  conn_list_iterate(game.est_connections, pconn) {
    if (pconn->player) {
      unattach_connection_from_player(pconn);
    }

    players_iterate(pplayer) {
      if (strcmp(pconn->username, pplayer->username) == 0) {
        attach_connection_to_player(pconn, pplayer);
        break;
      }
    } players_iterate_end;
  } conn_list_iterate_end;
  return TRUE;
}

/**************************************************************************
  used to load maps in pregamestate
**************************************************************************/
static bool loadmap_command(struct connection *caller, char *str,
                            bool check)
{
  struct section_file secfile;
  char buf[512], name[256];
  const char *p, *comment;
  bool isnumber = TRUE;

  if (server_state != PRE_GAME_STATE) {
    cmd_reply(CMD_LOADMAP, caller, C_FAIL,
              _("Can't load a new map while a game is running."));
    return FALSE;
  }

  if (map_is_loaded()) {
    cmd_reply(CMD_LOADMAP, caller, C_FAIL,
              _("Can't load a new map when one is already loaded."));
    return FALSE;
  }

  if (!str || str[0] == '\0') {
    cmd_reply(CMD_LOADMAP, caller, C_SYNTAX,
              _("Usage: loadmap <filename> or loadmap <number>"));
    return FALSE;
  }

  sz_strlcpy(buf, str);
  remove_leading_trailing_spaces(buf);
  for (p = buf; *p; p++) {
    if (!my_isdigit(*p)) {
      isnumber = FALSE;
      break;
    }
  }

  if (isnumber) {
    int mapnum;
    struct datafile_list *files;
    bool map_found = FALSE;
    int counter = 0;

    sscanf(buf, "%d", &mapnum);
    files = datafilelist_infix("maps", ".map", TRUE);

    datafile_list_iterate(files, pfile) {
      counter++;
      if (counter == mapnum) {
        sz_strlcpy(buf, pfile->fullname);
        sz_strlcpy(name, pfile->name);
        map_found = TRUE;
        break;
      }
    } datafile_list_iterate_end;
    free_datafile_list(files);

    if (mapnum > counter && !map_found) {
      cmd_reply(CMD_LOADMAP, caller, C_FAIL,
                _("There are only %d maps in the maplist."), counter);
      return FALSE;
    }

    if (!map_found) {
      cmd_reply(CMD_LOADMAP, caller, C_FAIL, _("There is no map %d."),
                mapnum);
      return FALSE;
    }

  } else {
    char datapath[512];
    const char *fullname;

    if (caller && caller->access_level != ALLOW_HACK
        && (strchr(buf, '/') || buf[0] == '.')) {
      cmd_reply(CMD_LOADMAP, caller, C_SYNTAX,
                _("You are not allowed to use this command."));
      return FALSE;
    }

    my_snprintf(datapath, sizeof(datapath), "maps/%s.map", buf);
    sz_strlcpy(name, buf);
    fullname = datafilename(datapath);
    if (fullname) {
      sz_strlcpy(buf, fullname);
    }
  }

  /* attempt to parse the file */
  if (!section_file_load_nodup(&secfile, buf)) {
    cmd_reply(CMD_LOADMAP, caller, C_FAIL, _("Couldn't load mapfile: %s"),
              buf);
    return FALSE;
  }

  if (check) {
    section_file_free(&secfile);
    return TRUE;
  }

  sz_strlcpy(srvarg.load_filename, buf);

  map_free();
  /* XXX Old code path had this, but is this necessary? */
  server_free_player_maps();
  map_init();

  game_loadmap(&secfile);

  section_file_check_unused(&secfile, buf);
  comment = secfile_lookup_str_default(&secfile,
                                       "No comment available",
                                       "game.comment");
  section_file_free(&secfile);

  settings_reset();

  sanity_check();

  notify_conn(NULL, _("Server: Map %s loaded: %s."), name, comment);

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool unloadmap_command(struct connection *caller, bool check)
{
  if (server_state != PRE_GAME_STATE) {
    cmd_reply(CMD_UNLOADMAP, caller, C_FAIL,
              _("Can't unload a map while a game is running."));
    return FALSE;
  }

  if (!map_is_loaded()) {
    cmd_reply(CMD_UNLOADMAP, caller, C_FAIL, _("No map loaded."));
    return FALSE;
  }

  /* Cannot unload a savegame. */
  if (!game.server.is_new_game) {
    cmd_reply(CMD_UNLOADMAP, caller, C_FAIL,
              _("You cannot unload a game loaded from a save file."));
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  map_free();

  /* FIXME: Old code path had this, but is this necessary? */
  server_free_player_maps();

  map_init();
  settings_reset();

  notify_conn(NULL, _("Server: Map unloaded."));
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static bool reset_command(struct connection *caller, bool check)
{
  if (check) {
    return TRUE;
  }

  reload_settings();

  notify_conn(NULL, _("Server: Settings re-initialized."));
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool showmaplist_command(struct connection *caller)
{
  struct datafile_list *files;
  struct section_file mapfile;
  const char *comment;
  int i;
  files = datafilelist_infix("maps", ".map", TRUE);

  i = files ? datafile_list_size(files) : 0;
  if (i <= 0) {
    cmd_reply(CMD_LIST, caller, C_FAIL,
              _("There are no maps in the maps directory."));
    free_datafile_list(files);
    return FALSE;
  }

  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  cmd_reply(CMD_LIST, caller, C_COMMENT,
            _("You can load the following %d map(s) with the command"), i);
  cmd_reply(CMD_LIST, caller, C_COMMENT,
            /* TRANS: do not translate "/loadmap" */
            _("/loadmap <mapnumber> or /loadmap <mapfile-name>:"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  i = 0;
  datafile_list_iterate(files, pfile) {
    i++;
    if (!section_file_load_nodup(&mapfile, pfile->fullname)) {
      cmd_reply(CMD_LIST, caller, C_FAIL,
                _("%d: (Failed to open secfile %s.)"), i, pfile->fullname);
      continue;
    }

    comment = secfile_lookup_str_default(&mapfile,
                                         "No comment available",
                                         "game.comment");
    cmd_reply(CMD_LIST, caller, C_COMMENT, _("%d: %s"), i, pfile->name);
    cmd_reply(CMD_LIST, caller, C_COMMENT, "  %s", comment);
  } datafile_list_iterate_end;
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  free_datafile_list(files);
  return TRUE;
}

/**************************************************************************
  test if this file a scenario
**************************************************************************/
static bool is_scenario(struct section_file *pfile, char *buf,
                        size_t buf_len)
{
  if (buf) {
    buf[0] = '\0';
  }

  if (secfile_lookup_int_default(pfile, 0, "game.version")
      < REQUIERED_GAME_VERSION) {
    if (buf) {
      my_snprintf(buf, buf_len, _("too old version"));
    }
    return FALSE;
  }
  if (secfile_lookup_bool_default(pfile, FALSE, "game.save_players")) {
    if (buf) {
      my_snprintf(buf, buf_len, _("this is a save game"));
    }
    return FALSE;
  }

  return TRUE;
}

/**************************************************************************
  build the list of the scenarios
**************************************************************************/
static struct datafile_list *get_scenario_list(void)
{
  struct datafile_list *files =
      datafilelist_infix("scenario", ".sav", TRUE);
  struct section_file file;

  datafile_list_iterate(files, pfile) {
    if (section_file_load_nodup(&file, pfile->fullname)) {
      if (!is_scenario(&file, NULL, 0)) {
        datafile_list_unlink(files, pfile);
        free(pfile);
      }
      section_file_free(&file);
    } else {
      datafile_list_unlink(files, pfile);
      free(pfile);
    }
  } datafile_list_iterate_end;

  return files;
}

/**************************************************************************
  used to load scenarios in pregamestate
**************************************************************************/
static bool loadscenario_command(struct connection *caller, char *str,
                                 bool check)
{
  struct timer *loadtimer, *uloadtimer;
  struct section_file secfile;
  char buf[512], name[256], message[256];
  const char *p;
  bool isnumber = TRUE;
  int old_nplayers;

  if (server_state != PRE_GAME_STATE) {
    cmd_reply(CMD_LOADSCERARIO, caller, C_FAIL,
              _("Can't load a new scenario while a game is running."));
    return FALSE;
  }

  if (map_is_loaded()) {
    cmd_reply(CMD_LOADSCERARIO, caller, C_FAIL,
              _("Can't load a new scenario when one is already loaded."));
    return FALSE;
  }

  if (!str || str[0] == '\0') {
    cmd_reply(CMD_LOADSCERARIO, caller, C_SYNTAX,
              _("Usage: loadscenario <filename> or loadscenario <number>"));
    return FALSE;
  }

  sz_strlcpy(buf, str);
  remove_leading_trailing_spaces(buf);
  for (p = buf; *p; p++) {
    if (!my_isdigit(*p)) {
      isnumber = FALSE;
      break;
    }
  }
  if (isnumber) {
    int mapnum;
    struct datafile_list *files;
    bool map_found = FALSE;
    int counter = 0;

    sscanf(buf, "%d", &mapnum);
    files = get_scenario_list();
    datafile_list_iterate(files, pfile) {
      counter++;
      if (counter == mapnum) {
        sz_strlcpy(buf, pfile->fullname);
        sz_strlcpy(name, pfile->name);
        map_found = TRUE;
        break;
      }
    } datafile_list_iterate_end;
    free_datafile_list(files);

    if (mapnum > counter && !map_found) {
      cmd_reply(CMD_LOADMAP, caller, C_FAIL,
                _("There are only %d scenarios in the scenario list."),
                counter);
      return FALSE;
    }

    if (!map_found) {
      cmd_reply(CMD_LOADMAP, caller, C_FAIL, _("There is no scenario %d."),
                mapnum);
      return FALSE;
    }

  } else {
    char datapath[512];
    const char *fullname;

    if (caller && caller->access_level != ALLOW_HACK
        && (strchr(buf, '/') || buf[0] == '.')) {
      cmd_reply(CMD_LOADMAP, caller, C_SYNTAX,
                _("You are not allowed to use this command."));
      return FALSE;
    }

    my_snprintf(datapath, sizeof(datapath), "scenario/%s.sav", buf);
    sz_strlcpy(name, buf);
    fullname = datafilename(datapath);
    if (fullname) {
      sz_strlcpy(buf, fullname);
    }
  }
  /* attempt to parse the file */
  if (!section_file_load_nodup(&secfile, buf)) {
    cmd_reply(CMD_LOADMAP, caller, C_FAIL, _("Couldn't load scenario: %s."),
              buf);
    return FALSE;
  }
  if (!is_scenario(&secfile, message, sizeof(message))) {
    cmd_reply(CMD_LOADMAP, caller, C_FAIL, _("Cannot load %s: %s."),
              buf, message);
    section_file_free(&secfile);
    return FALSE;
  }

  if (check) {
    section_file_free(&secfile);
    return TRUE;
  }

  map_free();
  /* FIXME: Old code path had this, but is this necessary? */
  server_free_player_maps();
  map_init();

  loadtimer = new_timer_start(TIMER_CPU, TIMER_ACTIVE);
  uloadtimer = new_timer_start(TIMER_USER, TIMER_ACTIVE);
  sz_strlcpy(srvarg.load_filename, buf);

  /* NB If the scenario has 0 players, game_load will set
     game.info.nplayers to 0. So save the current players by
     remembering the old value of game.info.nplayers and restoring
     it after. */
  /* XXX Is there no less hackish way to do this? :( */
  old_nplayers = game.info.nplayers;
  game_load(&secfile);

  /* XXX Do we have to restore these? */
  if (game.info.nplayers == 0) {
    game.info.nplayers = old_nplayers;
  }
  game.server.is_new_game = TRUE;

  if ((p = secfile_lookup_str_default(&secfile, NULL, "game.metastring"))) {
    set_meta_message_string(p);
    if (is_metaserver_open()) {
      send_server_info_to_metaserver(META_INFO);
    }
  }

  section_file_check_unused(&secfile, buf);
  section_file_free(&secfile);
  freelog(LOG_VERBOSE, "Load time: %g seconds (%g apparent)",
          read_timer_seconds_free(loadtimer),
          read_timer_seconds_free(uloadtimer));

  reload_settings();

  sanity_check();

  notify_conn(NULL, _("Server: Scenario %s loaded."), name);

  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool set_rulesetdir(struct connection *caller, char *str, bool check)
{
  char filename[512], *pfilename, verror[256];

  sz_strlcpy(filename, str);
  remove_leading_trailing_spaces(filename);
  if ((str == NULL) || (strlen(str) == 0)) {
    cmd_reply(CMD_RULESETDIR, caller, C_SYNTAX,
              _("Current ruleset directory is \"%s\""),
              game.server.rulesetdir);
    return FALSE;
  } else if (caller && caller->access_level != ALLOW_HACK
             && (strchr(filename, '/') || filename[0] == '.')) {
    cmd_reply(CMD_RULESETDIR, caller, C_SYNTAX,
              _("You are not allowed to use this command."));
    return FALSE;
  }
  my_snprintf(filename, sizeof(filename), "%s", str);
  pfilename = datafilename(filename);
  if (!pfilename) {
    /* maybe an integer */
    int num, i;

    if (sscanf(str, "%d", &num) != 1) {
      cmd_reply(CMD_RULESETDIR, caller, C_SYNTAX,
                _("Ruleset directory \"%s\" not found"), str);
      return FALSE;
    }
    char **rulesets = get_rulesets_list();

    if (num >= 1 && num <= MAX_NUM_RULESETS && rulesets[num - 1]) {
      sz_strlcpy(filename, rulesets[num - 1]);
      pfilename = datafilename(filename);
    }
    for (i = 0; rulesets[i] && i < MAX_NUM_RULESETS; i++) {
      free(rulesets[i]);
    }
    if (!pfilename) {
      cmd_reply(CMD_RULESETDIR, caller, C_SYNTAX,
                _("%d is not a valid ruleset id"), num);
      return FALSE;
    }
  }
  if (!is_valid_ruleset(filename, verror, sizeof(verror))) {
    cmd_reply(CMD_RULESETDIR, caller, C_SYNTAX,
              _("\"%s\" is not a valid directory: %s"), filename, verror);
    return FALSE;
  }
  if (!check) {
    cmd_reply(CMD_RULESETDIR, caller, C_OK,
              _("Ruleset directory set to \"%s\""), filename);
    sz_strlcpy(game.server.rulesetdir, filename);
  }

  return TRUE;
}

/**************************************************************************
  Cutting away a trailing comment by putting a '\0' on the '#'. The
  method handles # in single or double quotes. It also takes care of
  "\#".
**************************************************************************/
static void cut_comment(char *str)
{
  int i;
  bool in_single_quotes = FALSE, in_double_quotes = FALSE;
  freelog(LOG_DEBUG, "cut_comment(str='%s')", str);
  for (i = 0; i < strlen(str); i++) {
    if (str[i] == '"' && !in_single_quotes) {
      in_double_quotes = !in_double_quotes;
    } else if (str[i] == '\'' && !in_double_quotes) {
      in_single_quotes = !in_single_quotes;
    } else if (str[i] == '#' && !(in_single_quotes || in_double_quotes)
               && (i == 0 || str[i - 1] != '\\')) {
      str[i] = '\0';
      break;
    }
  }
  freelog(LOG_DEBUG, "cut_comment: returning '%s'", str);
}

/**************************************************************************
...
**************************************************************************/
static bool quit_game(struct connection *caller, bool check)
{
  if (!check) {
    cmd_reply(CMD_QUIT, caller, C_OK, _("Goodbye."));
    gamelog(GAMELOG_JUDGE, GL_NONE);
    gamelog(GAMELOG_END);
    server_quit();
  }
  return TRUE;
}

/**************************************************************************
  Handle "command input", which could really come from stdin on console,
  or from client chat command, or read from file with -r, etc.
  caller==NULL means console, str is the input, which may optionally
  start with SERVER_COMMAND_PREFIX character.

  If check is TRUE, then do nothing, just check syntax.
**************************************************************************/
bool handle_stdin_input(struct connection * caller,
                        const char *str, bool check)
{
  char command[MAX_LEN_CONSOLE_LINE], arg[MAX_LEN_CONSOLE_LINE];
  char allargs[MAX_LEN_CONSOLE_LINE], full_command[MAX_LEN_CONSOLE_LINE];
  const char *cptr_s;
  char *cptr_d;
  int i;
  enum command_id cmd;
  enum cmdlevel_id level;
  struct setting_value sv;


  /* notify to the server console */
  if (!check && caller) {
    con_write(C_COMMENT, "%s: '%s'", caller->username, str);
  }

  /* if the caller may not use any commands at all, don't waste any time */
  if (may_use_nothing(caller)) {
    cmd_reply(CMD_HELP, caller, C_FAIL,
              _("Sorry, you are not allowed to use server commands."));
    return FALSE;
  }

  /* Is it a comment or a blank line? */
  /* line is comment if the first non-whitespace character is '#': */
  cptr_s = skip_leading_spaces(str);
  if (*cptr_s == '\0' || *cptr_s == '#') {
    return FALSE;
  }

  /* commands may be prefixed with SERVER_COMMAND_PREFIX, even when
     given on the server command line - rp */
  if (*cptr_s == SERVER_COMMAND_PREFIX) {
    cptr_s++;
  }
  while (*cptr_s != '\0' && !my_isalnum(*cptr_s)) {
    cptr_s++;
  }

  /* copy the full command, in case we need it for voting purposes. */
  sz_strlcpy(full_command, cptr_s);

  /* cptr_s points now to the beginning of the real command. It has
   * skipped leading whitespace, the SERVER_COMMAND_PREFIX and any
   * other non-alphanumeric characters.
   */
  for (cptr_d = command; *cptr_s != '\0' && my_isalnum(*cptr_s) &&
       cptr_d < command + sizeof(command) - 1; cptr_s++, cptr_d++) {
    *cptr_d = *cptr_s;
  }
  *cptr_d = '\0';

  cmd = command_named(command, FALSE);
  if (cmd == CMD_AMBIGUOUS) {
    cmd = command_named(command, TRUE);
    cmd_reply(cmd, caller, C_SYNTAX,
              _("Warning: '%s' interpreted as '%s', but it is ambiguous."
                "  Try '%shelp'."),
              command, commands[cmd].name, caller ? "/" : "");
  } else if (cmd == CMD_UNRECOGNIZED) {
    cmd_reply(cmd, caller, C_SYNTAX,
              _("Unknown command.  Try '%shelp'."), caller ? "/" : "");
    return FALSE;
  }

  if (conn_is_muted(caller) && cmd != CMD_START_GAME) {
    cmd_reply(cmd, caller, C_REJECTED,
              _("Since you are muted you are not allowed to use "
                "this command."));
    return FALSE;
  }

  level = command_access_level(cmd);

  cptr_s = skip_leading_spaces(cptr_s);
  sz_strlcpy(arg, cptr_s);


  cut_comment(arg);

  /* keep this before we cut everything after a space */
  sz_strlcpy(allargs, cptr_s);
  cut_comment(allargs);

  i = strlen(arg) - 1;
  while (i > 0 && my_isspace(arg[i])) {
    arg[i--] = '\0';
  }


  if (cmd == CMD_SET) {
    parse_set_arguments(arg, &sv);
  }

  if (connection_can_vote(caller)
      && !check && (caller->access_level == ALLOW_BASIC
                    || (commands[cmd].vote_flags & VCF_ALWAYSVOTE))
      && level == ALLOW_CTRL) {
    struct vote *vote;

    /* If we already have a vote going, cancel it in favour of the new
     * vote command. You can only have one vote at a time. */
    if (get_vote_by_caller(caller)) {
      cmd_reply(CMD_VOTE, caller, C_COMMENT,
                _("Your new vote cancelled your previous vote."));
    }

    /* Check if the vote command would succeed. */
    if (handle_stdin_input(caller, full_command, TRUE)
        && (vote = vote_new(caller, allargs, cmd, &sv))) {
      char votedesc[MAX_LEN_CONSOLE_LINE];

      describe_vote(vote, votedesc, sizeof(votedesc));

      if (cmd == CMD_POLL) {
        notify_conn(NULL, _("New poll (vote %d) by %s: %s"),
                    vote->vote_no, caller->username,
                    votedesc);
      } else {
        notify_conn(NULL, _("New vote (number %d) by %s: %s"),
                    vote->vote_no, caller->username,
                    votedesc);
      }

      /* Vote on your own suggestion. */
      connection_vote(caller, vote, VOTE_YES);
      return TRUE;

    } else {
      cmd_reply(CMD_VOTE, caller, C_FAIL,
                _("Your new vote (\"%s\") was not "
                  "legal or was not recognized."), full_command);
      return FALSE;
    }
  }

  if (caller && !(check && caller->access_level >= ALLOW_BASIC
                  && level == ALLOW_CTRL)
      && caller->access_level < level) {
    cmd_reply(cmd, caller, C_FAIL,
              _("You are not allowed to use this command."));
    return FALSE;
  }


  if (!check) {
    struct conn_list *echolist = conn_list_new();

    switch (commands[cmd].echo_mode) {

    case ECHO_NONE:
      break;

    case ECHO_USER:
      if (caller)
        conn_list_append(echolist, caller);
      break;

    case ECHO_PLAYERS:
      conn_list_iterate(game.est_connections, pconn) {
        if (pconn->player && !pconn->observer) {
          conn_list_append(echolist, pconn);
        }
      } conn_list_iterate_end;
      break;

    case ECHO_ADMINS:
      conn_list_iterate(game.est_connections, pconn) {
        if (pconn->access_level >= ALLOW_ADMIN) {
          conn_list_append(echolist, pconn);
        }
      } conn_list_iterate_end;
      break;

    case ECHO_ALL:
      conn_list_iterate(game.est_connections, pconn) {
        conn_list_append(echolist, pconn);
      } conn_list_iterate_end;
      break;

    default:
      assert(0 /* should not happend */ );
      break;
    }

    if (conn_list_size(echolist) > 0) {
      notify_conn(echolist, "%s: '%s %s'", caller ? caller->username
                  : _("(server prompt)"), command, arg);
    }
    conn_list_free(echolist);
  }

  switch (cmd) {
  case CMD_REMOVE:
    return remove_player(caller, arg, check);
  case CMD_SAVE:
    return save_command(caller, arg, check);
  case CMD_LOAD:
    return load_command(caller, arg, check);
  case CMD_LOADMAP:
    return loadmap_command(caller, arg, check);
  case CMD_UNLOADMAP:
    return unloadmap_command(caller, check);
  case CMD_LOADSCERARIO:
    return loadscenario_command(caller, arg, check);
  case CMD_METAPATCHES:
  case CMD_METATOPIC:
  case CMD_METAMESSAGE:
    return do_meta_set_command(caller, arg, check, cmd);
  case CMD_METACONN:
    return metaconnection_command(caller, arg, check);
  case CMD_METASERVER:
    return metaserver_command(caller, arg, check);
  case CMD_HELP:
    return show_help(caller, arg);
  case CMD_SRVID:
    return show_serverid(caller, arg);
  case CMD_LIST:
    return show_list(caller, arg);
  case CMD_AITOGGLE:
    return toggle_ai_player(caller, arg, check);
  case CMD_TAKE:
    return take_command(caller, arg, check);
  case CMD_OBSERVE:
    return observe_command(caller, arg, check);
  case CMD_DETACH:
    return detach_command(caller, arg, check);
  case CMD_CREATE:
    return create_ai_player(caller, arg, check);
  case CMD_AWAY:
    return set_away(caller, arg, check);
  case CMD_NOVICE:
    return set_ai_level(caller, arg, 2, check);
  case CMD_EASY:
    return set_ai_level(caller, arg, 3, check);
  case CMD_NORMAL:
    return set_ai_level(caller, arg, 5, check);
  case CMD_HARD:
    return set_ai_level(caller, arg, 7, check);
  case CMD_EXPERIMENTAL:
    return set_ai_level(caller, arg, 10, check);
  case CMD_QUIT:
    return quit_game(caller, check);
  case CMD_CUT:
    return cut_client_connection(caller, arg, check);
  case CMD_BAN:
    return ban_command(caller, arg, check);
  case CMD_UNBAN:
    return unban_command(caller, arg, check);
  case CMD_ADDACTION:
    return addaction_command(caller, arg, check);
  case CMD_DELACTION:
    return delaction_command(caller, arg, check);
  case CMD_LOADACTIONLIST:
    return loadactionlist_command(caller, arg, check);
  case CMD_SAVEACTIONLIST:
    return saveactionlist_command(caller, arg, check);
  case CMD_CLEARACTIONLIST:
    return clearactionlist_command(caller, arg, check);
  case CMD_SHOW:
    return show_command(caller, arg, check);
  case CMD_EXPLAIN:
    return explain_option(caller, arg, check);
  case CMD_DEBUG:
    return debug_command(caller, arg, check);
  case CMD_SET:
    return set_command(caller, &sv, check);
  case CMD_TEAM:
    return team_command(caller, arg, check);
  case CMD_RULESETDIR:
    return set_rulesetdir(caller, arg, check);
  case CMD_SCORE:
    if (server_state == RUN_GAME_STATE || server_state == GAME_OVER_STATE) {
      if (!check) {
        report_progress_scores();
      }
      return TRUE;
    } else {
      cmd_reply(cmd, caller, C_SYNTAX,
                _
                ("The game must be running before you can see the score."));
      return FALSE;
    }
  case CMD_WALL:
    return wall(arg, check);
  case CMD_VOTE:
    return vote_command(caller, arg, check);
  case CMD_POLL:
    return poll_command(caller, arg, check);
  case CMD_EMOTE:
    return emote_command(caller, arg, check);
  case CMD_CANCEL_VOTE:
    return cancel_vote_command(caller, arg, check);
  case CMD_READ_SCRIPT:
    return read_command(caller, arg, check);
  case CMD_WRITE_SCRIPT:
    return write_command(caller, arg, check);
  case CMD_RESET:
    return reset_command(caller, check);
  case CMD_WELCOME_MESSAGE:
    return welcome_message_command(caller, arg, check);
  case CMD_WELCOME_FILE:
    return welcome_file_command(caller, arg, check);
  case CMD_DNS_LOOKUP:
    return dnslookup_command(caller, arg, check);
  case CMD_REQUIRE:
    return require_command(caller, arg, check);
  case CMD_KICK:
    return kick_command(caller, arg, check);
  case CMD_RFCSTYLE:           /* see console.h for an explanation */
    if (!check) {
      con_set_style(!con_get_style());
    }
    return TRUE;
  case CMD_CMDLEVEL:
    return cmdlevel_command(caller, arg, check);
  case CMD_INCREMENTALTIMEOUT:
    return incremental_timeout_command(caller, allargs, check);
  case CMD_IGNORE:
    return ignore_command(caller, allargs, check);
  case CMD_UNIGNORE:
    return unignore_command(caller, allargs, check);
  case CMD_AUTOTEAM:
    return autoteam_command(caller, allargs, check);
  case CMD_MUTE:
    return mute_command(caller, allargs, check);
  case CMD_UNMUTE:
    return unmute_command(caller, allargs, check);
#ifdef HAVE_MYSQL
  case CMD_STATS:
    return stats_command(caller, allargs, check);
  case CMD_RATINGS:
    return ratings_command(caller, arg, check);
  case CMD_EXAMINE:
    return examine_command(caller, arg, check);
  case CMD_TOPTEN:
    return topten_command(caller, arg, check);
  case CMD_GAMELIST:
    return gamelist_command(caller, arg, check);
  case CMD_AKA:
    return aka_command(caller, arg, check);
  case CMD_AUTHDB:
    return authdb_command(caller, arg, check);
  case CMD_FCDB:
    return fcdb_command(caller, allargs, check);
#endif /* HAVE_MYSQL */
  case CMD_START_GAME:
    return start_command(caller, arg, check);
  case CMD_END_GAME:
    return end_command(caller, arg, check);
  case CMD_NUM:
  case CMD_UNRECOGNIZED:
  case CMD_AMBIGUOUS:
  default:
    freelog(LOG_FATAL,
            "bug in civserver: impossible command recognized; bye!");
    assert(0);
  }

  /* should NEVER happen but we need to satisfy some compilers */
  return FALSE;
}

/**************************************************************************
  End the game and accord victory to the listed players, if any.
**************************************************************************/
static bool end_command(struct connection *caller, char *str, bool check)
{
  char *arg[MAX_NUM_PLAYERS];
  int ntokens = 0, i;
  enum m_pre_result plr_result;
  bool result = TRUE, declared_winner = FALSE;
  char buf[MAX_LEN_CONSOLE_LINE];

  if (server_state != RUN_GAME_STATE) {
    cmd_reply(CMD_END_GAME, caller, C_FAIL,
              _("Cannot end the game: no game running."));
    return FALSE;
  }

  if (str != NULL || strlen(str) > 0) {
    sz_strlcpy(buf, str);
    ntokens = get_tokens(buf, arg, MAX_NUM_PLAYERS, TOKEN_DELIMITERS);
  }

  if (game.server.fcdb.type == GT_SOLO && ntokens > 0) {
    cmd_reply(CMD_END_GAME, caller, C_REJECTED,
              _("You can only win a solo game by building a spaceship!"));
    return FALSE;
  }

  /* Ensure players exist */
  for (i = 0; i < ntokens; i++) {
    struct player *pplayer =
        find_player_by_name_prefix(arg[i], &plr_result);
    if (!pplayer) {
      cmd_reply_no_such_player(CMD_TEAM, caller, arg[i], plr_result);
      result = FALSE;
      goto cleanup;
    } else if (pplayer->is_alive == FALSE) {
      cmd_reply(CMD_END_GAME, caller, C_FAIL, _("But %s is dead!"),
                pplayer->name);
      result = FALSE;
      goto cleanup;
    }
  }
  if (check) {
    goto cleanup;
  }
  if (ntokens > 0) {
    /* Mark players for victory. */
    for (i = 0; i < ntokens; i++) {
      BV_SET(srvarg.draw,
             find_player_by_name_prefix(arg[i], &plr_result)->player_no);
      declared_winner = TRUE;
    }
  }

  game.server.fcdb.outcome = GOC_ENDED_BY_VOTE;
  if (declared_winner) {
    players_iterate(pplayer) {
      if (BV_ISSET(srvarg.draw, pplayer->player_no)) {
        pplayer->result = PR_WIN;
        if (pplayer->team != TEAM_NONE) {
          players_iterate(pplayer_other) {
            if (pplayer->team == pplayer_other->team)
              pplayer_other->result = PR_WIN;
          } players_iterate_end;
        }
      } else if (pplayer->result == PR_NONE) {
        pplayer->result = PR_LOSE;
      }
    } players_iterate_end;
  }

  server_state = GAME_OVER_STATE;
  force_end_of_sniff = TRUE;
  cmd_reply(CMD_END_GAME, caller, C_OK,
            _("Ending the game. The server will restart once all clients "
              "have disconnected."));
cleanup:
  for (i = 0; i < ntokens; i++) {
    free(arg[i]);
  }
  return result;
}

/**************************************************************************
  Returns TRUE if the settings are suitable for a rated game or if the
  game is not rated (i.e. game.server.rated == FALSE). Default (i.e. standard)
  settings are only enforced for rated 'solo' games, since otherwise it
  is fine if players agree to play a rated game with non-standard
  settings. Of course server only settings and some others are exempt,
  e.g. 'timeout'.
**************************************************************************/
static bool check_settings_for_rated_game(void)
{
  /* Non-default settings that should be enforced
   * for SOLO games. */
  struct standard_int_setting {
    int *psetting;
    int standard_value;
  } sis[] = {
    { &map.autosize, 108},
    { &map.landpercent, 17},      /* landmass */
    { &game.ext_info.airliftingstyle, 1},
    { &game.traderoute_info.traderevenuestyle, 1},
    { &game.traderoute_info.caravanbonusstyle, 1},
    { &game.traderoute_info.trademindist, 8}
  };

  struct standard_bool_setting {
    bool *psetting;
    bool standard_value;
  } sbs[] = {
    { &map.tinyisles, TRUE},
    { &map.alltemperate, TRUE},
    { &game.ext_info.techtrading, FALSE},
    { &game.ext_info.goldtrading, FALSE},
    { &game.ext_info.citytrading, FALSE},
    { &game.ext_info.globalwarmingon, FALSE},
    { &game.ext_info.nuclearwinteron, FALSE},
    { &game.ext_info.stackbribing, TRUE},
    { &game.ext_info.experimentalbribingcost, TRUE}
  };

  struct standard_string_setting {
    const char *psetting;
    const char *standard_value;
  } sss[] = {
    { game.server.allow_take, "H4h1a1" }
  };

  struct settings_s *op;
  bool ok = TRUE, checked = FALSE;
  int i;
  enum game_types type = game_determine_type();

  if (!srvarg.fcdb.enabled || !srvarg.auth.enabled) {
    /* No sense in enforcing settings if we cannot update the
     * database or determine who logged in. */

    if (game.server.rated) {
      notify_conn(NULL, _("Server: This game cannot be rated because"
                          " database communication is disabled (option"
                          " 'rated' has been set to 0)."));
      game.server.rated = FALSE;
    }

    return TRUE;
  }

  if (!game.server.rated || type == GT_MIXED) {
    /* For unrated games or 'mixed' type games, any settings are ok. */
    return TRUE;
  }

  for (op = settings; op->name; op++) {
    if (op->to_client == SSET_SERVER_ONLY) {
      continue;
    }

    /* Use pointer comparison to check which setting we are
     * testing. Much faster than name comparison and requires
     * the same amount of maintenance if a setting definition
     * is changed. */

    /* Always allow these settings to be changed. */
    if (op->int_value == &game.info.timeout
        || op->int_value == &game.info.min_players
        || op->int_value == &game.info.max_players) {
      continue;
    }

    if (type == GT_TEAM && op->int_value == &game.info.diplomacy
        && *op->int_value != 4) {
      notify_conn(NULL, _("Game: Warning: Diplomacy is not disabled "
                          "for this team game."));
      continue;
    }

    if (type == GT_FFA && op->int_value == &game.ext_info.maxallies
        && *op->int_value > 1) {
      notify_conn(NULL, _("Game: Warning: The setting 'maxallies' "
                          "is greater than one, this may result in "
                          "an unfair free-for-all game."));
      continue;
    }

    if (type != GT_SOLO) {
      /* Non-default settings can only prevent a solo game from
       * being rated. */
      continue;
    }

    if (map_is_loaded()
        && (op->int_value == &map.size
            || op->int_value == &map.landpercent)) {
      /* When a game is reloaded, ignore map setting changes
       * due to autosize. */
      continue;
    }

    /* NB: What follows are the setting checks for SOLO games only. */

    if (op->int_value == &game.info.diplomacy
        || op->int_value == &game.ext_info.maxallies) {
      /* We don't care about these settings for solo games. */
      continue;
    }

    checked = FALSE;

    switch (op->type) {
    case SSET_BOOL:
      for (i = 0; i < ARRAY_SIZE(sbs); i++) {
        if (sbs[i].psetting == op->bool_value) {
          if (sbs[i].standard_value != *op->bool_value) {
            notify_conn(NULL, _("Game: The setting '%s' is not "
                                "at the standard value of %d."),
                        op->name, sbs[i].standard_value);
            ok = FALSE;
          }
          checked = TRUE;
          break;
        }
      }
      if (!checked && *op->bool_value != op->bool_default_value) {
        notify_conn(NULL, _("Game: The setting '%s' is not "
                            "at the default value of %d."),
                    op->name, op->bool_default_value);
        ok = FALSE;
      }
      break;

    case SSET_INT:
      for (i = 0; i < ARRAY_SIZE(sis); i++) {
        if (sis[i].psetting == op->int_value) {
          if (sis[i].standard_value != *op->int_value) {
            notify_conn(NULL, _("Game: The setting '%s' is not "
                                "at the standard value of %d."),
                        op->name, sis[i].standard_value);
            ok = FALSE;
          }
          checked = TRUE;
          break;
        }
      }
      if (!checked && *op->int_value != op->int_default_value) {
        notify_conn(NULL, _("Game: The setting '%s' is not "
                            "at the default value of %d."),
                    op->name, op->int_default_value);
        ok = FALSE;
      }
      break;

    case SSET_STRING:
      for (i = 0; i < ARRAY_SIZE(sss); i++) {
        if (sss[i].psetting == op->string_value) {
          if (0 != strcmp(sss[i].standard_value, op->string_value)) {
            notify_conn(NULL, _("Game: The setting '%s' is not "
                                "at the standard value of \"%s\"."),
                        op->name, sss[i].standard_value);
            ok = FALSE;
          }
          checked = TRUE;
          break;
        }
      }
      if (!checked
          && 0 != strcmp(op->string_value, op->string_default_value)) {
        notify_conn(NULL, _("Game: The setting '%s' is not "
                            "at the default value of \"%s\"."),
                    op->name, op->string_default_value);
        ok = FALSE;
      }
      break;

    default:
      assert(0);
      break;
    }
  }

  return ok;
}

/**************************************************************************
...
**************************************************************************/
static bool start_command(struct connection *caller, char *name, bool check)
{
  int started = 0, notstarted = 0;
  static int failed_rated_start = 0;
  const int MAX_FAILED_RATED_STARTS = 3;
  enum game_types type = GT_NUM_TYPES;

  switch (server_state) {

  case PRE_GAME_STATE:
    /* Sanity check scenario */
    if (game.server.is_new_game && !check) {
      if (map.num_start_positions > 0
          && game.info.max_players > map.num_start_positions) {
        /* If we load a pre-generated map (i.e., a scenario) it is possible
         * to increase the number of players beyond the number supported by
         * the scenario.  The solution is a hack: cut the extra players
         * when the game starts. */
        freelog(LOG_VERBOSE, _("Reduced maxplayers from %i to %i to fit "
                               "to the number of start positions."),
                game.info.max_players, map.num_start_positions);
        game.info.max_players = map.num_start_positions;
      }
      if (get_num_nonobserver_players() > game.info.max_players) {
        /* Because of the way player ids are renumbered during
           server_remove_player() this is correct */
        while (get_num_nonobserver_players() > game.info.max_players) {
          /* This may erronously remove observer players sometimes.  This
           * is a bug but non-fatal. */
          server_remove_player(get_player(game.info.max_players));
        }

        freelog(LOG_VERBOSE,
                _("Had to cut down the number of players to the "
                  "number of map start positions, there must be "
                  "something wrong with the savegame or you "
                  "adjusted the maxplayers value."));
      }
    }
    /* check min_players */
    if (get_num_nonobserver_players() < game.info.min_players) {
      cmd_reply(CMD_START_GAME, caller, C_FAIL,
                _("Not enough players, game will not start."));
      return FALSE;
    }

    if (!caller) {
      /* Forced start by server operator. */
      start_game();
      return TRUE;
    }

    if (!caller->player || caller->observer) {
      /* A detached or observer player can't do /start. */
      cmd_reply(CMD_START_GAME, caller, C_REJECTED,
                _("You are not a player in the game!"));
      return FALSE;
    }

    players_iterate(pplayer) {
      if (pplayer->is_connected) {
        if (pplayer->is_started) {
          started++;
        } else {
          notstarted++;
        }
      }
    } players_iterate_end;

    /* Note this is called even if the player has pressed /start once
     * before.  This is a good thing given that no other code supports
     * is_started yet.  For instance if a player leaves everyone left
     * might have pressed /start already but the start won't happen
     * until someone presses it again.  Also you can press start more
     * than once to remind other people to start (which is a good thing
     * until somebody does it too much and it gets labeled as spam). */

    /* Spam is bad. Use chat to remind others to start. */
    if (caller->player->is_started && started < started + notstarted) {
      cmd_reply(CMD_START_GAME, caller, C_COMMENT,
                _("You have already notified others that you are ready"
                  " to start."));
      return TRUE;
    }

    if (check) {
      return TRUE;
    }

    if (notstarted >= MAX_FAILED_RATED_STARTS) {
      failed_rated_start = 0;
    }

    /* If we are getting close to starting, check that settings
     * are ok for this type of rated game. */
    if (notstarted < 3 && !check_settings_for_rated_game()) {
      failed_rated_start++;
      if (failed_rated_start < MAX_FAILED_RATED_STARTS - 1) {
        notify_conn(NULL, _("Game: The game will not start unless "
                            "settings are fixed or the 'rated' setting "
                            "is set to 0 (this warning will repeat %d "
                            "more %s)."),
                    MAX_FAILED_RATED_STARTS - failed_rated_start - 1,
                    PL_("time", "times", MAX_FAILED_RATED_STARTS
                        - failed_rated_start - 1));
      } else if (failed_rated_start == MAX_FAILED_RATED_STARTS - 1) {
        notify_conn(NULL, _("Game: The game will not start unless "
                            "settings are fixed or the 'rated' setting "
                            "is set to 0."));
        notify_conn(NULL, _("Game: WARNING: Attempting to start again "
                            "without fixing settings will set the "
                            "'rated' option to 0)."));
      } else {
        notify_conn(NULL, _("Game: This game will not be rated because "
                            "settings are not suitable (option 'rated' "
                            "has been set to 0)."));
        game.server.rated = FALSE;
        failed_rated_start = 0;
      }
      return TRUE;
    }

    if (!caller->player->is_started) {
      caller->player->is_started = TRUE;
      started++;
      notstarted--;
    }

    type = game_determine_type();

    if (notstarted > 0) {
      notify_conn(NULL, _("Game: %s is ready. %d out of %d players are "
                          "ready to start a %s%s game."),
                  caller->username, started, started + notstarted,
                  game.server.rated ? _("RATED ") : "",
                  game_type_name(type));
      return TRUE;
    }

    if (game.server.rated) {
      notify_conn(NULL, _("Game: All players are ready; "
                          "starting a RATED %s game."),
                  game_type_name(type));
    } else {
      notify_conn(NULL, _("Game: All players are ready; "
                          "starting a %s game."),
                  game_type_name(type));
    }
    start_game();
    return TRUE;

  case GAME_OVER_STATE:
    /* TRANS: given when /start is invoked during gameover. */
    cmd_reply(CMD_START_GAME, caller, C_FAIL,
              _
              ("Cannot start the game: the game is waiting for all clients "
               "to disconnect."));
    return FALSE;

  case SELECT_RACES_STATE:
    /* TRANS: given when /start is invoked during nation selection. */
    cmd_reply(CMD_START_GAME, caller, C_FAIL,
              _("Cannot start the game: it has already been started."));
    return FALSE;

  case RUN_GAME_STATE:
    /* TRANS: given when /start is invoked while the game is running. */
    cmd_reply(CMD_START_GAME, caller, C_FAIL,
              _("Cannot start the game: it is already running."));
    return FALSE;

  default:
    break;
  }
  assert(FALSE);
  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
static bool addaction_command(struct connection *caller,
                              char *pattern, bool check)
{
  struct user_action *pua;
  int i, type, action;
  char *p, buf[1024], err[256];
  sz_strlcpy(buf, pattern);
  remove_leading_trailing_spaces(buf);
  if (!buf[0]) {
    cmd_reply(CMD_ADDACTION, caller, C_SYNTAX,
              _("Incorrect syntax. Try /help addaction."));
    return FALSE;
  }
  if (!(p = strchr(buf, ' '))) {
    cmd_reply(CMD_ADDACTION, caller, C_SYNTAX,
              _("Incorrect syntax. Try /help addaction."));
    return FALSE;
  }
  *p++ = 0;                     /* Mark end of <action> string */
  action = NUM_ACTION_TYPES;
  for (i = 0; i < NUM_ACTION_TYPES; i++) {
    if (!mystrcasecmp(buf, user_action_type_strs[i])) {
      action = i;
      break;
    }
  }
  if (action == NUM_ACTION_TYPES) {
    cmd_reply(CMD_ADDACTION, caller, C_SYNTAX,
              _("Invalid action. Try /help addaction."));
    return FALSE;
  }
  type = CPT_HOSTNAME;
  if (!parse_conn_pattern(p, buf, sizeof(buf), &type, err, sizeof(err))) {
    cmd_reply(CMD_ADDACTION, caller, C_SYNTAX,
              _("Incorrect syntax: %s. Try /help addaction."), err);
    return FALSE;
  }
  remove_leading_trailing_spaces(buf);
  if (!buf[0]) {
    cmd_reply(CMD_ADDACTION, caller, C_SYNTAX,
              _("The pattern cannot be empty!"));
    return FALSE;
  }
  if (check)
    return TRUE;
  pua = user_action_new(buf, type, action);
  user_action_list_append(on_connect_user_actions, pua);
  user_action_as_str(pua, buf, sizeof(buf));
  cmd_reply(CMD_ADDACTION, caller, C_COMMENT,
            _("Added %s to the action list."), buf);
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
void user_action_free(struct user_action *pua)
{
  if (pua->conpat) {
    conn_pattern_free(pua->conpat);
    pua->conpat = NULL;
  }
  free(pua);
}

/**************************************************************************
...
**************************************************************************/
static bool delaction_command(struct connection *caller,
                              char *pattern, bool check)
{
  char buf[1024];
  struct user_action *pua;
  int len, n;
  len = user_action_list_size(on_connect_user_actions);
  if (!len) {
    cmd_reply(CMD_DELACTION, caller, C_FAIL,
              _("The action list is emtpy."));
    return FALSE;
  }
  sz_strlcpy(buf, pattern);
  remove_leading_trailing_spaces(buf);
  n = atoi(buf);
  if (!(1 <= n && n <= len)) {
    cmd_reply(CMD_DELACTION, caller, C_FAIL,
              _("Invalid action list entry number, the allowed "
                "range is 1 to %d."), len);
    return FALSE;
  }
  if (check)
    return TRUE;
  pua = user_action_list_get(on_connect_user_actions, n - 1);
  user_action_as_str(pua, buf, sizeof(buf));
  user_action_list_unlink(on_connect_user_actions, pua);
  user_action_free(pua);

  cmd_reply(CMD_UNBAN, caller, C_COMMENT,
            _("Entry %d removed from the action list %s."), n, buf);
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static bool ban_command(struct connection *caller, char *pattern,
                        bool check)
{
  struct user_action *pua;
  int type = CPT_HOSTNAME;
  char buf[1024], err[256];
  if (pattern) {
    sz_strlcpy(buf, pattern);
    remove_leading_trailing_spaces(buf);
  }
  if (!pattern || !buf[0]) {
    cmd_reply(CMD_BAN, caller, C_FAIL,
              _("This command requires an an argument. Try /help ban."));
    return FALSE;
  }
  if (!parse_conn_pattern(pattern, buf, sizeof(buf), &type,
                          err, sizeof(err))) {
    cmd_reply(CMD_BAN, caller, C_SYNTAX,
              _("Incorrect syntax: %s. Try /help ban."), err);
    return FALSE;
  }
  if (check)
    return TRUE;
  pua = user_action_new(buf, type, ACTION_BAN);
  user_action_list_prepend(on_connect_user_actions, pua);
  conn_pattern_as_str(pua->conpat, buf, sizeof(buf));
  cmd_reply(CMD_BAN, caller, C_COMMENT, _("%s has been banned."), buf);
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static bool unban_command(struct connection *caller,
                          char *pattern, bool check)
{
  char buf[1024], err[256];
  bool found = FALSE;
  int len, type;
  len = user_action_list_size(on_connect_user_actions);
  if (!len) {
    cmd_reply(CMD_UNBAN, caller, C_FAIL, _("The action list is emtpy."));
    return FALSE;
  }
  type = CPT_HOSTNAME;
  if (!parse_conn_pattern(pattern, buf, sizeof(buf), &type,
                          err, sizeof(err))) {
    cmd_reply(CMD_UNBAN, caller, C_SYNTAX,
              _("Incorrect syntax: %s. Try /help unban."), err);
    return FALSE;
  }
  if (check)
    return TRUE;
  user_action_list_iterate(on_connect_user_actions, pua) {
    if (pua->action == ACTION_BAN && pua->conpat->type == type
        && 0 == mystrcasecmp(buf, pua->conpat->pattern)) {
      found = TRUE;
      user_action_list_unlink(on_connect_user_actions, pua);
      user_action_free(pua);
      break;
    }
  } user_action_list_iterate_end;
  if (found) {
    cmd_reply(CMD_UNBAN, caller, C_COMMENT,
              _("%s=%s has been unbanned."),
              conn_pattern_type_strs[type], buf);
  } else {
    cmd_reply(CMD_UNBAN, caller, C_FAIL,
              _("Ban pattern '%s' was not found in the list of "
                "user actions."), pattern);
  }
  return found;
}

/********************************************************************
  Simple command wrapper for load_ban_list.
*********************************************************************/
bool loadactionlist_command(struct connection * caller,
                            char *arg, bool check)
{
  char safename[512], *filename = NULL;
  int ret;
  if (check)
    return TRUE;
  if (arg) {
    sz_strlcpy(safename, arg);
    remove_leading_trailing_spaces(safename);
    filename = safename;
  }

  if (!filename || !filename[0])
    filename = DEFAULT_ACTION_LIST_FILE;

  ret = load_action_list(filename);
  if (ret == -1) {
    cmd_reply(CMD_LOADACTIONLIST, caller, C_FAIL,
              _("Could not load action list from %s."), filename);
    return FALSE;
  }
  cmd_reply(CMD_LOADACTIONLIST, caller, C_COMMENT,
            _("Added %d action(s) from %s to the action list."),
            ret, filename);
  return TRUE;
}

/********************************************************************
  Simple command wrapper for save_ban_list.
*********************************************************************/
bool saveactionlist_command(struct connection * caller,
                            char *filename, bool check)
{
  int ret;
  if (!filename || filename[0] == '\0')
    filename = DEFAULT_ACTION_LIST_FILE;
  ret = save_action_list(filename);
  if (ret == -1) {
    cmd_reply(CMD_SAVEACTIONLIST, caller, C_FAIL,
              _("Could not save action list to %s."), filename);
    return FALSE;
  }
  cmd_reply(CMD_SAVEACTIONLIST, caller, C_COMMENT,
            _("Saved %d action(s) from action list to %s."), ret, filename);
  return TRUE;
}

/********************************************************************
  ...
*********************************************************************/
bool clearactionlist_command(struct connection * caller,
                             char *filename, bool check)
{
  if (check)
    return TRUE;
  clear_all_on_connect_user_actions();
  cmd_reply(CMD_CLEARACTIONLIST, caller, C_COMMENT,
            _("Action list cleared."));
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static int get_action_list_file_version(const char *line)
{
  static const char *VERSTR = "version=";
  int version;
  if (strncmp(VERSTR, line, strlen(VERSTR)))
    return 0;
  version = atoi(line + strlen(VERSTR));
  return version > 0 ? version : 0;
}

/**************************************************************************
  ...
**************************************************************************/
static int load_action_list_v0(const char *filename)
{
  FILE *file;
  char line[1024], *p;
  char addr[1024];
  int actionid;
  int len, count;
  if (!(file = fopen(filename, "r"))) {
    freelog(LOG_ERROR, "Could not open action list file %s: %s",
            filename, mystrerror(myerrno()));
    return -1;
  }
  count = 0;
  while (fgets(line, sizeof(line), file)) {
    len = strlen(line);
    if (len <= 0)
      continue;
    /* Remove leading and trailing whitespace */
    p = line + len - 1;
    while (p != line && my_isspace(*p))
      *p-- = '\0';
    p = line;
    while (*p != '\0' && my_isspace(*p))
      p++;
    if (strlen(p) <= 0)
      continue;
    /* Skip comments */
    if (*p == '#')
      continue;
    if (sscanf(p, "%s %d", addr, &actionid) == 2) {
      struct user_action *pua;

      if (actionid < 0 || actionid >= NUM_ACTION_TYPES)
        continue;

      pua = user_action_new(!strcmp(addr, "ALL") ? "*" : addr,
                            CPT_HOSTNAME, actionid);
      user_action_list_append(on_connect_user_actions, pua);
    } else
      continue;
    count++;
  }
  fclose(file);
  freelog(LOG_VERBOSE, "Loaded %d action list item%s from %s",
          count, count == 0 || count > 1 ? "s" : "", filename);
  return count;
}

/**************************************************************************
  ...
**************************************************************************/
static int load_action_list_v1(const char *filename)
{
  FILE *file;
  char line[1024], *p, pat[512], err[128];
  int action, type;
  int count = 0, lc = 0, i;
  struct user_action *pua;
  int ver;
  if (!(file = fopen(filename, "r"))) {
    freelog(LOG_ERROR, _("Could not open action list file %s: %s."),
            filename, mystrerror(myerrno()));
    return -1;
  }
  /* Version line */
  fgets(line, sizeof(line), file);
  ver = get_action_list_file_version(line);
  if (ver != ACTION_LIST_FILE_VERSION) {
    freelog(LOG_ERROR, _("Unrecognized action list file version: "
                         "got %d, but expected %d."),
            ver, ACTION_LIST_FILE_VERSION);
    return -1;
  }
  while (fgets(line, sizeof(line), file)) {
    lc++;
    if ((p = strchr(line, '#')))
      *p = 0;
    remove_leading_trailing_spaces(line);
    if (strlen(line) <= 0)
      continue;
    if (!(p = strchr(line, ' '))) {
      freelog(LOG_ERROR, _("Syntax error on line %d of "
                           "action list file %s: action part of rule not "
                           "found."), lc, filename);
      continue;
    }
    *p++ = 0;

    type = CPT_HOSTNAME;
    if (!parse_conn_pattern(p, pat, sizeof(pat), &type, err, sizeof(err))) {
      freelog(LOG_ERROR, _("Syntax error on line %d of "
                           "action list file %s: %s."), lc, filename, p);
      continue;
    }

    action = NUM_ACTION_TYPES;
    remove_trailing_spaces(line);
    if (my_isdigit(*line)) {
      action = atoi(line);
    } else {
      for (i = 0; i < NUM_ACTION_TYPES; i++) {
        if (!mystrcasecmp(line, user_action_type_strs[i])) {
          action = i;
          break;
        }
      }
    }
    if (action < 0 || action >= NUM_ACTION_TYPES) {
      freelog(LOG_ERROR, _("Syntax error on line %d of "
                           "action list file %s: unrecognized action \"%s\"."),
              lc, filename, line);
      continue;
    }
    pua = user_action_new(pat, type, action);
    user_action_list_append(on_connect_user_actions, pua);
    count++;
  }
  fclose(file);
  freelog(LOG_VERBOSE, "Loaded %d action%s from %s (version %d)",
          count, count == 0 || count > 1 ? "s" : "", filename, ver);
  return count;
}

/**************************************************************************
 Read the hostnames in the given filename and adds them to the action list.
 Assumes that action has been initialized.
**************************************************************************/
static int load_action_list(const char *filename)
{
  FILE *file;
  char line[64];
  int version;
  if (!(file = fopen(filename, "r"))) {
    freelog(LOG_ERROR, _("Could not open action list file %s: %s"),
            filename, mystrerror(myerrno()));
    return -1;
  }
  fgets(line, sizeof(line), file);
  fclose(file);
  version = get_action_list_file_version(line);
  if (version == 0) {
    return load_action_list_v0(filename);
  } else if (version == ACTION_LIST_FILE_VERSION) {
    return load_action_list_v1(filename);
  }
  freelog(LOG_ERROR, _("Unrecognized action list file version for "
                       "file %s: %d"), filename, version);
  return -1;
}

/**************************************************************************
  Saves the hostnames in actionlist to a file.
**************************************************************************/
static int save_action_list(const char *filename)
{
  FILE *file;
  int len = 0;

  if (!(file = fopen(filename, "w"))) {
    freelog(LOG_ERROR, "Could not save action list to %s because open"
            " failed: %s", filename, mystrerror(myerrno()));
    return -1;
  }
  fprintf(file, "version=%d\n", ACTION_LIST_FILE_VERSION);
  user_action_list_iterate(on_connect_user_actions, pua) {
    fprintf(file, "%s %s=%s\n",
            user_action_type_strs[pua->action],
            conn_pattern_type_strs[pua->conpat->type],
            pua->conpat->pattern);
    len++;
  } user_action_list_iterate_end;
  fclose(file);
  freelog(LOG_VERBOSE, "Saved %d action list item%s to %s",
          len, len == 0 || len > 1 ? "s" : "", filename);
  return len;
}

/**************************************************************************
  ...
**************************************************************************/
static bool cut_client_connection(struct connection *caller, char *name,
                                  bool check)
{
  enum m_pre_result match_result;
  struct connection *ptarget;
  struct player *pplayer;
  bool was_connected;

  ptarget = find_conn_by_user_prefix(name, &match_result);
  if (!ptarget) {
    cmd_reply_no_such_conn(CMD_CUT, caller, name, match_result);
    return FALSE;
  } else if (check) {
    return TRUE;
  }
  pplayer = ptarget->player;
  was_connected = pplayer ? pplayer->is_connected : FALSE;
  cmd_reply(CMD_CUT, caller, C_DISCONNECTED,
            _("Cutting connection %s."), ptarget->username);
  server_break_connection(ptarget, ES_CUT_COMMAND);
  /* if we cut the connection, unassign the login name */
  if (pplayer && was_connected && !pplayer->is_connected) {
    sz_strlcpy(pplayer->username, ANON_USER_NAME);
  }
  return TRUE;
}

/**************************************************************************
 Show caller introductory help about the server.
 help_cmd is the command the player used.
**************************************************************************/
static void show_help_intro(struct connection *caller,
                            enum command_id help_cmd)
{
  /* This is formated like extra_help entries for settings and commands: */
  const char *help =
      _("Welcome - this is the introductory help text for the Freeciv"
        "server.\n\n"
        "Two important server concepts are Commands and Options.\n"
        "Commands, such as 'help', are used to interact with the server.\n"
        "Some commands take one or more arguments, separated by spaces.\n"
        "In many cases commands and command arguments may be abbreviated.\n"
        "Options are settings which control the server as it is "
        "running.\n\n"
        "To find out how to get more information about commands and "
        "options,\n"
        "use 'help help'.\n\n"
        "For the impatient, the main commands to get going are:\n"
        "  show   -  to see current options\n" "  set    -  to set "
        "options\n"
        "  start  -  to start the game once players have connected\n"
        "  save   -  to save the current game\n" "  quit   -  to exit");
  static struct astring abuf = ASTRING_INIT;
  astr_minsize(&abuf, strlen(help) + 1);
  strcpy(abuf.str, help);
  wordwrap_string(abuf.str, 78);
  cmd_reply(help_cmd, caller, C_COMMENT, abuf.str);
}

/**************************************************************************
 Show the caller detailed help for the single COMMAND given by id.
 help_cmd is the command the player used.
**************************************************************************/
static void show_help_command(struct connection *caller,
                              enum command_id help_cmd, enum command_id id)
{
  const struct command *cmd = &commands[id];

  if (cmd->short_help) {
    cmd_reply(help_cmd, caller, C_COMMENT, "%s %s  -  %s",
              _("Command:"), cmd->name, _(cmd->short_help));
  } else {
    cmd_reply(help_cmd, caller, C_COMMENT, "%s %s",
              _("Command:"), cmd->name);
  }

  if (cmd->synopsis) {
    /* line up the synopsis lines: */
    const char *syn = _("Synopsis: ");
    size_t synlen = strlen(syn);
    char prefix[40];

    my_snprintf(prefix, sizeof(prefix), "%*s", (int) synlen, " ");
    cmd_reply_prefix(help_cmd, caller, C_COMMENT, prefix,
                     "%s%s", syn, _(cmd->synopsis));
  }

  if (cmd->game_level < ALLOW_NEVER) {
    cmd_reply(help_cmd, caller, C_COMMENT,
              _("Level: %s"), cmdlevel_name(cmd->game_level));
  }

  if (cmd->game_level != cmd->pregame_level
      && cmd->pregame_level < ALLOW_NEVER) {
    cmd_reply(help_cmd, caller, C_COMMENT, _("Pregame level: %s"),
              cmdlevel_name(cmd->pregame_level));
  }

  if ((cmd->game_level == ALLOW_CTRL || cmd->pregame_level == ALLOW_CTRL)
      && (cmd->vote_flags > VCF_NONE || cmd->vote_percent > 0)) {
    char buf[1024];
    buf[0] = '\0';

    if (cmd->vote_percent > 0) {
      cat_snprintf(buf, sizeof(buf),
                   _("Requires more than %d%% in favor to pass. "),
                   cmd->vote_percent);
    }

    if (cmd->vote_flags & VCF_NODISSENT) {
      sz_strlcat(buf, _("May only pass if there are no "
                        "dissenting votes. "));
    }

    if (cmd->vote_flags & VCF_UNANIMOUS) {
      sz_strlcat(buf, _("May only pass by unanimous vote. "));
    }

    if (cmd->vote_flags & VCF_WAITFORALL) {
      sz_strlcat(buf, _("The vote remains until all have voted "
                        "or the voting period elapses. "));
    }

    if (cmd->vote_flags & VCF_ALWAYSVOTE) {
      sz_strlcat(buf, _("A vote will be started even if the user "
                        "has an access level greater than 'basic'. "));
    }

    if (cmd->vote_flags & VCF_NOPASSALONE) {
      sz_strlcat(buf, _("If a vote could pass with only one voter "
                        "in favor, then the percent required is "
                        "increased until at least two votes in favor "
                        "are needed. "));
    }
    wordwrap_string(buf, 76);
    cmd_reply(help_cmd, caller, C_COMMENT, _("Voting:"));
    cmd_reply_prefix(help_cmd, caller, C_COMMENT, "  ", "  %s", buf);
  }

  if (cmd->extra_help) {
    static struct astring abuf = ASTRING_INIT;
    const char *help = _(cmd->extra_help);
    astr_minsize(&abuf, strlen(help) + 1);
    strcpy(abuf.str, help);
    wordwrap_string(abuf.str, 76);
    cmd_reply(help_cmd, caller, C_COMMENT, _("Description:"));
    cmd_reply_prefix(help_cmd, caller, C_COMMENT, "  ", "  %s", abuf.str);
  }
}

/**************************************************************************
 Show the caller list of COMMANDS.
 help_cmd is the command the player used.
**************************************************************************/
static void show_help_command_list(struct connection *caller,
                                   enum command_id help_cmd)
{
  enum command_id i;
  char buf[MAX_LEN_CONSOLE_LINE];
  int j;

  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
  cmd_reply(help_cmd, caller, C_COMMENT,
            _("The following server commands are available:"));
  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);

  buf[0] = '\0';
  for (i = 0, j = 0; i < CMD_NUM; i++) {
    if (may_use(caller, i)) {
      cat_snprintf(buf, sizeof(buf), "%-19s", commands[i].name);
      if ((++j % 4) == 0) {
        cmd_reply(help_cmd, caller, C_COMMENT, buf);
        buf[0] = '\0';
      }
    }
  }
  if (buf[0] != '\0')
    cmd_reply(help_cmd, caller, C_COMMENT, buf);
  if (caller && caller->player && !caller->observer
      && caller->access_level == ALLOW_BASIC) {
    cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
    cmd_reply(help_cmd, caller, C_COMMENT,
              _("The following server commands require a vote:"));
    cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);

    buf[0] = '\0';
    for (i = 0, j = 0; i < CMD_NUM; i++) {
      enum cmdlevel_id level = command_access_level(i);

      if (level == ALLOW_CTRL) {
        cat_snprintf(buf, sizeof(buf), "%-19s", commands[i].name);
        if ((++j % 4) == 0) {
          cmd_reply(help_cmd, caller, C_COMMENT, buf);
          buf[0] = '\0';
        }
      }
    }
    if (buf[0] != '\0')
      cmd_reply(help_cmd, caller, C_COMMENT, buf);
  }
  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
}

/**************************************************************************
  Additional 'help' arguments
**************************************************************************/
enum HELP_GENERAL_ARGS
{ HELP_GENERAL_COMMANDS, HELP_GENERAL_OPTIONS,
  HELP_GENERAL_NUM              /* Must be last */
};
static const char *const help_general_args[] = {
  "commands", "options", NULL
};

/**************************************************************************
  Unified indices for help arguments:
    CMD_NUM           -  Server commands
    HELP_GENERAL_NUM  -  General help arguments, above
    SETTINGS_NUM      -  Server options 
**************************************************************************/
#define HELP_ARG_NUM (CMD_NUM + HELP_GENERAL_NUM + SETTINGS_NUM)

/**************************************************************************
  Convert unified helparg index to string; see above.
**************************************************************************/
static const char *helparg_accessor(int i)
{
  if (i < CMD_NUM)
    return cmdname_accessor(i);
  i -= CMD_NUM;
  if (i < HELP_GENERAL_NUM)
    return help_general_args[i];
  i -= HELP_GENERAL_NUM;
  return optname_accessor(i);
}

/**************************************************************************
...
**************************************************************************/
static bool show_help(struct connection *caller, char *arg)
{
  enum m_pre_result match_result;
  int ind;
  assert(!may_use_nothing(caller));
  /* no commands means no help, either */
  match_result = match_prefix(helparg_accessor, HELP_ARG_NUM, 0,
                              mystrncasecmp, arg, &ind);
  if (match_result == M_PRE_EMPTY) {
    show_help_intro(caller, CMD_HELP);
    return FALSE;
  }
  if (match_result == M_PRE_AMBIGUOUS) {
    cmd_reply(CMD_HELP, caller, C_FAIL,
              _("Help argument '%s' is ambiguous."), arg);
    return FALSE;
  }
  if (match_result == M_PRE_FAIL) {
    cmd_reply(CMD_HELP, caller, C_FAIL,
              _("No match for help argument '%s'."), arg);
    return FALSE;
  }
  /* other cases should be above */
  assert(match_result < M_PRE_AMBIGUOUS);
  if (ind < CMD_NUM) {
    show_help_command(caller, CMD_HELP, ind);
    return TRUE;
  }
  ind -= CMD_NUM;
  if (ind == HELP_GENERAL_OPTIONS) {
    show_help_option_list(caller, CMD_HELP);
    return TRUE;
  }
  if (ind == HELP_GENERAL_COMMANDS) {
    show_help_command_list(caller, CMD_HELP);
    return TRUE;
  }
  ind -= HELP_GENERAL_NUM;
  if (ind < SETTINGS_NUM) {
    show_help_option(caller, CMD_HELP, ind);
    return TRUE;
  }
  /* should have finished by now */
  freelog(LOG_ERROR, "Bug in show_help!");
  return FALSE;
}

/**************************************************************************
  ...
**************************************************************************/
static void show_ignore(struct connection *caller)
{
  int n = 1;
  char buf[128];
  if (ignore_list_size(caller->server.ignore_list) <= 0) {
    cmd_reply(CMD_LIST, caller, C_COMMENT, _("Your ignore list is empty."));
    return;
  }
  cmd_reply(CMD_LIST, caller, C_COMMENT, _("Your ignore list:"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  ignore_list_iterate(caller->server.ignore_list, cp) {
    conn_pattern_as_str(cp, buf, sizeof(buf));
    cmd_reply(CMD_LIST, caller, C_COMMENT, "%d: %s", n++, buf);
  } ignore_list_iterate_end;
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

#ifdef HAVE_MYSQL
/**************************************************************************
  ...
**************************************************************************/
static void show_team_ratings(struct connection *caller, bool send_to_all)
{
  enum game_types type = GT_NUM_TYPES;
  char fmt[128];
  int cml_name;

  if (team_count() <= 0) {
    return;
  }

  /* This will usually be GT_TEAM, but
   * it could be GT_MIXED too, or some
   * future game type. */
  type = game_determine_type();

  /* Only reload ratings from the database if we are in
   * pregame (when teams could change easily). */
  if (server_state == PRE_GAME_STATE
      && (type == GT_NUM_TYPES
          || !fcdb_load_player_ratings(type, FALSE))) {
    /* Simply don't print anything more. */
    return;
  }

  score_assign_groupings();
  score_calculate_grouping_ratings();

  cml_name = 10;
  team_iterate(pteam) {
    cml_name = MAX(cml_name, strlen(get_team_name(pteam->id)) + 2);
  } team_iterate_end;

  my_snprintf(fmt, sizeof(fmt), "%s%%-%ds %%-10s %%-10s",
              send_to_all ? "Server: " : "", cml_name);

  if (send_to_all) {
    notify_conn(NULL, fmt, _("Team Name"), _("Rating"), _("RD"));
  } else {
    cmd_reply(CMD_LIST, caller, C_COMMENT, fmt,
              _("Team Name"), _("Rating"), _("RD"));
  }

  my_snprintf(fmt, sizeof(fmt), "%s%%-%ds %%-10.2f %%-10.2f",
              send_to_all ? "Server: " : "", cml_name);

  team_iterate(pteam) {
    if (send_to_all) {
      notify_conn(NULL, fmt, get_team_name(pteam->id),
                  pteam->fcdb.rating, pteam->fcdb.rating_deviation);
    } else {
      cmd_reply(CMD_LIST, caller, C_COMMENT, fmt,
                get_team_name(pteam->id), pteam->fcdb.rating,
                pteam->fcdb.rating_deviation);
    }
  } team_iterate_end;

  if (!send_to_all) {
    cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  }
    
}
#endif /* HAVE_MYSQL */

/**************************************************************************
  ...
**************************************************************************/
static void show_teams(struct connection *caller, bool send_to_all)
{
  bool listed[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
  int i, teamid = -1, count = 0, tc = 0;
  struct player *pplayer;
  char buf[1024], buf2[1024];
  memset(listed, 0, sizeof(bool) * (MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS));

  if (send_to_all) {
    notify_conn(NULL, _("Server: List of teams:"));
  } else {
    cmd_reply(CMD_LIST, caller, C_COMMENT, _("List of teams:"));
    cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  }
  while (count < game.info.nplayers) {
    for (i = 0; i < game.info.nplayers; i++) {
      if (listed[i]) {
        continue;
      }
      pplayer = get_player(i);
      if (teamid == -1) {
        teamid = pplayer->team;
        tc = 0;
        buf2[0] = 0;
      }
      if (teamid == pplayer->team) {
        count++;
        listed[i] = TRUE;
        tc++;
        cat_snprintf(buf2, sizeof(buf2), "<%s> ",
                     pplayer->is_connected ? pplayer->username : pplayer->
                     name);
      }
    }
    if (teamid != -1) {
      const char *teamname = teamid == TEAM_NONE
          ? _("Unassigned") : get_team_name(teamid);
      my_snprintf(buf, sizeof(buf), "%s (%d): %s", teamname, tc, buf2);
      if (send_to_all) {
        notify_conn(NULL, "Server:   %s", buf);
      } else {
        cmd_reply(CMD_LIST, caller, C_COMMENT, "%s", buf);
      }
      teamid = -1;
    }
  }
  if (!send_to_all) {
    cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  }

#ifdef HAVE_MYSQL
  show_team_ratings(caller, send_to_all);
#endif /* HAVE_MYSQL */
}

/**************************************************************************
  ...
**************************************************************************/
static void show_rulesets(struct connection *caller)
{
  char **rulesets = get_rulesets_list(), *description, *p;
  int i;

  cmd_reply(CMD_LIST, caller, C_COMMENT, _("List of rulesets available:"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);

  for (i = 0; i < MAX_NUM_RULESETS && rulesets[i]; i++) {
    description = get_ruleset_description(rulesets[i]);
    if (description && (p = strchr(description, '\n'))) {
      *p++ = '\0';
    } else {
      p = NULL;
    }
    cmd_reply(CMD_LIST, caller, C_COMMENT, "%d: %s%s%s", i + 1, rulesets[i],
              description ? ": " : "", description ? description : "");
    for (description = p; description; description = p) {
      if ((p = strchr(description, '\n'))) {
        *p++ = '\0';
      }
      cmd_reply(CMD_LIST, caller, C_COMMENT, "%*s%s",
                (int) strlen(rulesets[i]) + 5, "", description);
    }
    free(rulesets[i]);
  }

  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

/**************************************************************************
  ...
**************************************************************************/
static bool show_scenarios(struct connection *caller)
{
  struct datafile_list *files;
  int i;

  files = get_scenario_list();
  i = files ? datafile_list_size(files) : 0;

  if (i <= 0) {
    cmd_reply(CMD_LIST, caller, C_FAIL,
              _("There are no scernarios in the scernario directory."));
    free_datafile_list(files);
    return FALSE;
  }

  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  cmd_reply(CMD_LIST, caller, C_COMMENT,
            PL_("You can load the following %d scernario with the command",
                "You can load the following %d scernarios with the command",
                i), i);
  /* TRANS: do not translate "/loadscenario" */
  cmd_reply(CMD_LIST, caller, C_COMMENT,
            _("/loadscenario <scernarionumber> "
              "or /loadscenario <scernariofile-name>"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  i = 0;
  datafile_list_iterate(files, pfile) {
    i++;
    cmd_reply(CMD_LIST, caller, C_COMMENT, _("%d: %s"), i, pfile->name);
  } datafile_list_iterate_end;
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  free_datafile_list(files);
  return TRUE;
}

/**************************************************************************
  ...
**************************************************************************/
static bool show_mutes(struct connection *caller)
{
  struct connection *pconn;
  if (hash_num_entries(mute_table) < 1) {
    cmd_reply(CMD_LIST, caller, C_COMMENT, _("There are no muted users."));
    return TRUE;
  }

  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  cmd_reply(CMD_LIST, caller, C_COMMENT, "%-32s %s",
            _("Username / Address"), _("Turns left"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  hash_iterate(mute_table, void *, key, struct muteinfo *, mi) {
    pconn = find_conn_by_id(mi->conn_id);
    if (mi->turns_left > 0) {
      cmd_reply(CMD_LIST, caller, C_COMMENT, _("%-32s %d"),
                pconn ? pconn->username : mi->addr, mi->turns_left);
    } else {
      cmd_reply(CMD_LIST, caller, C_COMMENT, _("%-32s forever"),
                pconn ? pconn->username : mi->addr);
    }
  } hash_iterate_end;
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  return TRUE;
}

/**************************************************************************
  'list' arguments
**************************************************************************/
enum LIST_ARGS
{
  LIST_PLAYERS, LIST_CONNECTIONS, LIST_ACTIONLIST,
  LIST_TEAMS, LIST_IGNORE, LIST_MAPS, LIST_SCENARIOS,
  LIST_RULESETS, LIST_MUTES,
  LIST_ARG_NUM                  /* Must be last */
};
static const char *const list_args[] = {
  "players", "connections", "actionlist", "teams", "ignore",
  "maps", "scenarios", "rulesets", "mutes",
  NULL
};
static const char *listarg_accessor(int i)
{
  return list_args[i];
}

/**************************************************************************
  Show list of players or connections, or connection statistics.
**************************************************************************/
static bool show_list(struct connection *caller, char *arg)
{
  enum m_pre_result match_result;
  int ind;
  remove_leading_trailing_spaces(arg);
  match_result = match_prefix(listarg_accessor,
                              LIST_ARG_NUM, 0, mystrncasecmp, arg, &ind);
  if (match_result > M_PRE_EMPTY) {
    cmd_reply(CMD_LIST, caller, C_SYNTAX,
              _("Bad list argument: '%s'.  Try '%shelp list'."),
              arg, (caller ? "/" : ""));
    return FALSE;
  }
  if (match_result == M_PRE_EMPTY) {
    ind = LIST_PLAYERS;
  }
  switch (ind) {
  case LIST_PLAYERS:
    show_players(caller);
    return TRUE;
  case LIST_CONNECTIONS:
    show_connections(caller);
    return TRUE;
  case LIST_ACTIONLIST:
    show_actionlist(caller);
    return TRUE;
  case LIST_TEAMS:
    show_teams(caller, FALSE);
    return TRUE;
  case LIST_IGNORE:
    show_ignore(caller);
    return TRUE;
  case LIST_MAPS:
    return showmaplist_command(caller);
  case LIST_SCENARIOS:
    return show_scenarios(caller);
  case LIST_RULESETS:
    show_rulesets(caller);
    return TRUE;
  case LIST_MUTES:
    show_mutes(caller);
    return TRUE;
  default:
    cmd_reply(CMD_LIST, caller, C_FAIL,
              "Internal error: ind %d in show_list", ind);
    freelog(LOG_ERROR, "Internal error: ind %d in show_list", ind);
    return FALSE;
  }
}

/**************************************************************************
  ...
**************************************************************************/
void show_players(struct connection *caller)
{
  char buf[MAX_LEN_CONSOLE_LINE], buf2[MAX_LEN_CONSOLE_LINE];
  int n;

  cmd_reply(CMD_LIST, caller, C_COMMENT, _("List of players (%d):"),
            game.info.nplayers);
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);

  if (game.info.nplayers == 0) {
    cmd_reply(CMD_LIST, caller, C_WARNING, _("<no players>"));
  } else {
    players_iterate(pplayer) {
      /* Low access level callers don't get to see barbarians in list: */
      if (is_barbarian(pplayer) && caller
          && (caller->access_level < ALLOW_CTRL)) {
        continue;
      }

      /* buf2 contains stuff in brackets after playername:
       * [username,] AI/Barbarian/Human [,Dead] [, skill level] [, nation]
       */
      buf2[0] = '\0';
      if (strlen(pplayer->username) > 0
          && strcmp(pplayer->username, "nouser") != 0) {
        my_snprintf(buf2, sizeof(buf2), _("user %s, "), pplayer->username);
      }

      if (is_barbarian(pplayer)) {
        sz_strlcat(buf2, _("Barbarian"));
      } else if (pplayer->ai.control) {
        sz_strlcat(buf2, _("AI"));
      } else {
        sz_strlcat(buf2, _("Human"));
      }
      if (!pplayer->is_alive) {
        sz_strlcat(buf2, _(", Dead"));
      }
      if (pplayer->ai.control) {
        cat_snprintf(buf2, sizeof(buf2), _(", difficulty level %s"),
                     name_of_skill_level(pplayer->ai.skill_level));
      }
      if (server_state != PRE_GAME_STATE
          && pplayer->nation != NO_NATION_SELECTED) {
        cat_snprintf(buf2, sizeof(buf2), _(", nation %s"),
                     get_nation_name_plural(pplayer->nation));
      }
      if (pplayer->team != TEAM_NONE) {
        cat_snprintf(buf2, sizeof(buf2), _(", team %s"),
                     get_team_name(pplayer->team));
      }
      if (server_state == PRE_GAME_STATE && pplayer->is_connected) {
        if (pplayer->is_started) {
          cat_snprintf(buf2, sizeof(buf2), _(", ready"));
        } else {
          cat_snprintf(buf2, sizeof(buf2), _(", not ready"));
        }
      }
      my_snprintf(buf, sizeof(buf), "%s (%s)", pplayer->name, buf2);
      n = conn_list_size(pplayer->connections);

      if (n > 0) {
        cat_snprintf(buf, sizeof(buf),
                     PL_(" %d connection:", " %d connections:", n), n);
      }
      cmd_reply(CMD_LIST, caller, C_COMMENT, "%s", buf);
      conn_list_iterate(pplayer->connections, pconn) {
        if (!pconn->used) {
          /* A bug that we haven't been able to trace leaves unused
           * connections on the lists.  We skip them. */
          continue;
        }
        my_snprintf(buf, sizeof(buf),
                    _("  %s from %s (%s access), bufsize=%dkb"),
                    pconn->username, pconn->addr,
                    cmdlevel_name(pconn->access_level),
                    (pconn->send_buffer->nsize >> 10));
        if (pconn->observer) {
          sz_strlcat(buf, _(" (observer mode)"));
        }
        cmd_reply(CMD_LIST, caller, C_COMMENT, "%s", buf);
      } conn_list_iterate_end;
    } players_iterate_end;
  }
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

/**************************************************************************
  List connections; initially mainly for debugging
**************************************************************************/
static void show_connections(struct connection *caller)
{
  int n;

  n = conn_list_size(game.all_connections);

  cmd_reply(CMD_LIST, caller, C_COMMENT,
            _("List of connections to server (%d):"), n);
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);

  if (n == 0) {
    cmd_reply(CMD_LIST, caller, C_WARNING, _("<no connections>"));

  } else {
    char buf[MAX_LEN_CONSOLE_LINE], timebuf[128];
    time_t now, idle;

    now = time(NULL);

    conn_list_iterate(game.all_connections, pconn) {
      sz_strlcpy(buf, conn_description(pconn));
      if (pconn->established) {
        cat_snprintf(buf, sizeof(buf), _(" %s access"),
                     cmdlevel_name(pconn->access_level));
      }
      
      idle = now - pconn->server.idle_time;
      if (idle > 0) {
        format_time_duration(idle, timebuf, sizeof(timebuf));
        cat_snprintf(buf, sizeof(buf), _(" (idle %s)"), timebuf);
      }

      cmd_reply(CMD_LIST, caller, C_COMMENT, "%s", buf);
      cmd_reply(CMD_LIST, caller, C_COMMENT, _("  capabilities: %s"),
                pconn->capability);
    } conn_list_iterate_end;
  }

  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

/**************************************************************************
  List bans; show all hostnames in the ban list
**************************************************************************/
static void show_actionlist(struct connection *caller)
{
  int n, i = 1;
  char buf[256];

  /* NB caller == NULL implies the command was requested
     from the server console */
  if (caller && caller->access_level < ALLOW_ADMIN) {
    cmd_reply(CMD_LIST, caller, C_FAIL,
              _("You are not allowed to use this command."));
    return;
  }

  n = user_action_list_size(on_connect_user_actions);
  if (n == 0) {
    cmd_reply(CMD_LIST, caller, C_COMMENT, _("The action list is empty."));
    return;
  }
  cmd_reply(CMD_LIST, caller, C_COMMENT,
            _("Action list contents (%d):"), n);
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  user_action_list_iterate(on_connect_user_actions, pua) {
    user_action_as_str(pua, buf, sizeof(buf));
    cmd_reply(CMD_LIST, caller, C_COMMENT, "%d %s", i++, buf);
  } user_action_list_iterate_end;
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

#ifdef HAVE_LIBREADLINE
/********************* RL completion functions ***************************/
/* To properly complete both commands, player names, options and filenames
   there is one array per type of completion with the commands that
   the type is relevant for.
*/

/**************************************************************************
  A generalised generator function: text and state are "standard"
  parameters to a readline generator function;
  num is number of possible completions, and index2str is a function
  which returns each possible completion string by index.
**************************************************************************/
static char *generic_generator(const char *text, int state, int num,
                               const char *(*index2str) (int))
{
  static int list_index, len;
  const char *name;
  char *mytext = local_to_internal_string_malloc(text);

  /* This function takes a string (text) in the local format and must return
   * a string in the local format.  However comparisons are done against
   * names that are in the internal format (UTF-8).  Thus we have to convert
   * the text function from the local to the internal format before doing
   * the comparison, and convert the string we return *back* to the
   * local format when returning it. */

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (state == 0) {
    list_index = 0;
    len = strlen(mytext);
  }
  /* Return the next name which partially matches: */
  while (list_index < num) {
    name = index2str(list_index);
    list_index++;
    if (mystrncasecmp(name, mytext, len) == 0) {
      free(mytext);
      return internal_to_local_string_malloc(name);
    }
  }
  free(mytext);

  /* If no names matched, then return NULL. */
  return ((char *) NULL);
}

/**************************************************************************
The valid commands at the root of the prompt.
**************************************************************************/
static char *command_generator(const char *text, int state)
{
  return generic_generator(text, state, CMD_NUM, cmdname_accessor);
}

/**************************************************************************
The valid arguments to "set" and "explain"
**************************************************************************/
static char *option_generator(const char *text, int state)
{
  return generic_generator(text, state, SETTINGS_NUM, optname_accessor);
}

/**************************************************************************
  The valid arguments to "show"
**************************************************************************/
static char *olevel_generator(const char *text, int state)
{
  return generic_generator(text, state, SETTINGS_NUM + SSET_NUM_LEVELS,
                           olvlname_accessor);
}

/**************************************************************************
The player names.
**************************************************************************/
static const char *playername_accessor(int idx)
{
  return get_player(idx)->name;
}
static char *player_generator(const char *text, int state)
{
  return generic_generator(text, state, game.info.nplayers,
                           playername_accessor);
}

/**************************************************************************
The connection user names, from game.all_connections.
**************************************************************************/
static const char *connection_name_accessor(int idx)
{
  return conn_list_get(game.all_connections, idx)->username;
}
static char *connection_generator(const char *text, int state)
{
  return generic_generator(text, state,
                           conn_list_size(game.all_connections),
                           connection_name_accessor);
}

/**************************************************************************
The valid arguments for the first argument to "cmdlevel".
Extra accessor function since cmdlevel_name() takes enum argument, not int.
**************************************************************************/
static const char *cmdlevel_arg1_accessor(int idx)
{
  return cmdlevel_name(idx);
}
static char *cmdlevel_arg1_generator(const char *text, int state)
{
  return generic_generator(text, state, ALLOW_NUM, cmdlevel_arg1_accessor);
}

/**************************************************************************
The valid arguments for the second argument to "cmdlevel":
"first" or "new" or a connection name.
**************************************************************************/
static const char *cmdlevel_arg2_accessor(int idx)
{
  return ((idx == 0) ? "first" :
          (idx == 1) ? "new" : connection_name_accessor(idx - 2));
}
static char *cmdlevel_arg2_generator(const char *text, int state)
{
  return generic_generator(text, state,
                           2 + conn_list_size(game.all_connections),
                           cmdlevel_arg2_accessor);
}

/**************************************************************************
The valid first arguments to "help".
**************************************************************************/
static char *help_generator(const char *text, int state)
{
  return generic_generator(text, state, HELP_ARG_NUM, helparg_accessor);
}

/**************************************************************************
The valid first arguments to "list".
**************************************************************************/
static char *list_generator(const char *text, int state)
{
  return generic_generator(text, state, LIST_ARG_NUM, listarg_accessor);
}

/**************************************************************************
returns whether the characters before the start position in rl_line_buffer
is of the form [non-alpha]*cmd[non-alpha]*
allow_fluff changes the regexp to [non-alpha]*cmd[non-alpha].*
**************************************************************************/
static bool contains_str_before_start(int start, const char *cmd,
                                      bool allow_fluff)
{
  char *str_itr = rl_line_buffer;
  int cmd_len = strlen(cmd);

  while (str_itr < rl_line_buffer + start && !my_isalnum(*str_itr))
    str_itr++;

  if (mystrncasecmp(str_itr, cmd, cmd_len) != 0)
    return FALSE;
  str_itr += cmd_len;
  if (my_isalnum(*str_itr))     /* not a distinct word */
    return FALSE;
  if (!allow_fluff) {
    for (; str_itr < rl_line_buffer + start; str_itr++)
      if (my_isalnum(*str_itr))
        return FALSE;
  }

  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static bool is_command(int start)
{
  char *str_itr;
  if (contains_str_before_start(start, commands[CMD_HELP].name, FALSE))
    return TRUE;
  /* if there is only it is also OK */
  str_itr = rl_line_buffer;
  while (str_itr - rl_line_buffer < start) {
    if (my_isalnum(*str_itr))
      return FALSE;
    str_itr++;
  }
  return TRUE;
}

/**************************************************************************
Commands that may be followed by a player name
**************************************************************************/
static const int player_cmd[] = {
  CMD_AITOGGLE,
  CMD_NOVICE,
  CMD_EASY,
  CMD_NORMAL,
  CMD_HARD,
  CMD_EXPERIMENTAL,
  CMD_REMOVE,
  CMD_TEAM,
  -1
};

/**************************************************************************
number of tokens in rl_line_buffer before start
**************************************************************************/
static int num_tokens(int start)
{
  int res = 0;
  bool alnum = FALSE;
  char *chptr = rl_line_buffer;
  while (chptr - rl_line_buffer < start) {
    if (my_isalnum(*chptr)) {
      if (!alnum) {
        alnum = TRUE;
        res++;
      }
    } else {
      alnum = FALSE;
    }
    chptr++;
  }

  return res;
}

/**************************************************************************
...
**************************************************************************/
static bool is_player(int start)
{
  int i = 0;
  while (player_cmd[i] != -1) {
    if (contains_str_before_start
        (start, commands[player_cmd[i]].name, FALSE)) {
      return TRUE;
    }
    i++;
  }

  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
static bool is_connection(int start)
{
  return contains_str_before_start(start, commands[CMD_CUT].name, FALSE);
}

/**************************************************************************
...
**************************************************************************/
static bool is_cmdlevel_arg2(int start)
{
  return (contains_str_before_start
          (start, commands[CMD_CMDLEVEL].name, TRUE)
          && num_tokens(start) == 2);
}

/**************************************************************************
...
**************************************************************************/
static bool is_cmdlevel_arg1(int start)
{
  return contains_str_before_start(start, commands[CMD_CMDLEVEL].name,
                                   FALSE);
}

/**************************************************************************
  Commands that may be followed by a server option name
  CMD_SHOW is handled by option_level_cmd, which is for both option levels
  and server options
**************************************************************************/
static const int server_option_cmd[] = {
  CMD_EXPLAIN,
  CMD_SET,
  -1
};

/**************************************************************************
  Returns TRUE if the readline buffer string matches a server option at
  the given position.
**************************************************************************/
static bool is_server_option(int start)
{
  int i = 0;
  while (server_option_cmd[i] != -1) {
    if (contains_str_before_start
        (start, commands[server_option_cmd[i]].name, FALSE)) {
      return TRUE;
    }
    i++;
  }

  return FALSE;
}

/**************************************************************************
  Commands that may be followed by an option level or server option
**************************************************************************/
static const int option_level_cmd[] = {
  CMD_SHOW,
  -1
};

/**************************************************************************
  Returns true if the readline buffer string matches an option level or an
  option at the given position.
**************************************************************************/
static bool is_option_level(int start)
{
  int i = 0;
  while (option_level_cmd[i] != -1) {
    if (contains_str_before_start(start, commands[option_level_cmd[i]].name,
                                  FALSE)) {
      return TRUE;
    }
    i++;
  }

  return FALSE;
}

/**************************************************************************
Commands that may be followed by a filename
**************************************************************************/
static const int filename_cmd[] = {
  CMD_LOADACTIONLIST,
  CMD_SAVEACTIONLIST,
  CMD_LOAD,
  CMD_SAVE,
  CMD_READ_SCRIPT,
  CMD_WRITE_SCRIPT,
  CMD_WELCOME_FILE,
  -1
};

/**************************************************************************
...
**************************************************************************/
static bool is_filename(int start)
{
  int i = 0;
  while (filename_cmd[i] != -1) {
    if (contains_str_before_start
        (start, commands[filename_cmd[i]].name, FALSE)) {
      return TRUE;
    }
    i++;
  }

  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
static bool is_help(int start)
{
  return contains_str_before_start(start, commands[CMD_HELP].name, FALSE);
}

/**************************************************************************
...
**************************************************************************/
static bool is_list(int start)
{
  return contains_str_before_start(start, commands[CMD_LIST].name, FALSE);
}

/**************************************************************************
Attempt to complete on the contents of TEXT.  START and END bound the
region of rl_line_buffer that contains the word to complete.  TEXT is
the word to complete.  We can use the entire contents of rl_line_buffer
in case we want to do some simple parsing.  Return the array of matches,
or NULL if there aren't any.
**************************************************************************/
#ifdef HAVE_NEWLIBREADLINE
char **freeciv_completion(const char *text, int start, int end)
#else
char **freeciv_completion(char *text, int start, int end)
#endif
{
  char **matches = (char **) NULL;
  if (is_help(start)) {
    matches = completion_matches(text, help_generator);
  } else if (is_command(start)) {
    matches = completion_matches(text, command_generator);
  } else if (is_list(start)) {
    matches = completion_matches(text, list_generator);
  } else if (is_cmdlevel_arg2(start)) {
    matches = completion_matches(text, cmdlevel_arg2_generator);
  } else if (is_cmdlevel_arg1(start)) {
    matches = completion_matches(text, cmdlevel_arg1_generator);
  } else if (is_connection(start)) {
    matches = completion_matches(text, connection_generator);
  } else if (is_player(start)) {
    matches = completion_matches(text, player_generator);
  } else if (is_server_option(start)) {
    matches = completion_matches(text, option_generator);
  } else if (is_option_level(start)) {
    matches = completion_matches(text, olevel_generator);
  } else if (is_filename(start)) {
    /* This function we get from readline */
    matches = completion_matches(text, filename_completion_function);
  } else                        /* We have no idea what to do */
    matches = NULL;
  /* Don't automatically try to complete with filenames */
  rl_attempted_completion_over = 1;
  return (matches);
}
#endif /* HAVE_LIBREADLINE */
