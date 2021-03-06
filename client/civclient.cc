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
#  include "../config.hh"
#endif

#ifdef WIN32_NATIVE
#  include <winsock2.h>
#  include <windows.h>    /* LoadLibrary() */
#endif

#ifdef SDL
#  include "SDL/SDL.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "wc_iconv.hh"
#include "wc_intl.hh"
#include "log.hh"
#include "mem.hh"
#include "netintf.hh"
#include "rand.hh"
#include "support.hh"

#include "capstr.hh"
#include "dataio.hh"
#include "game.hh"
#include "diptreaty.hh"
#include "idex.hh"
#include "map.hh"
#include "packets.hh"
#include "version.hh"

#include "attribute.hh"
#include "audio.hh"
#include "cityrepdata.hh"
#include "climisc.hh"
#include "clinet.hh"
#include "agents/cma_core.hh"     /* kludge */
#include "connectdlg_common.hh"  /* client_kill_server() */
#include "control.hh"
#include "goto.hh"
#include "helpdata.hh"           /* boot_help_texts() */
#include "multiselect.hh"
#include "options.hh"
#include "packhand.hh"
#include "tilespec.hh"
#include "trade.hh"

#include "agents/agents.hh"

#include "include/chatline_g.hh"
#include "include/citydlg_g.hh"
#include "include/connectdlg_g.hh"
#include "include/dialogs_g.hh"
#include "include/diplodlg_g.hh"
#include "include/graphics_g.hh"
#include "include/gui_main_g.hh"
#include "include/mapctrl_g.hh"
#include "include/mapview_g.hh"
#include "include/menu_g.hh"
#include "include/messagewin_g.hh"
#include "include/pages_g.hh"
#include "include/plrdlg_g.hh"
#include "include/repodlgs_g.hh"

#include "civclient.hh"


/* The address and port of the server we are
 * currenly connected or trying to connect to. */
char server_host[512] = "\0";
int server_port = -1;

/* this is used in strange places, and is 'extern'd where
   needed (hence, it is not 'extern'd in civclient.h) */
bool is_server = FALSE;
time_t end_of_turn;

char *logfile = NULL;
char *scriptfile = NULL;
bool auto_connect = FALSE; /* TRUE = skip "Connect to Warciv Server" dialog */
bool do_not_request_hack = FALSE;

static enum client_states client_state = CLIENT_BOOT_STATE;

/* TRUE if an end turn request is blocked by busy agents */
bool waiting_for_end_turn = FALSE;

/*
 * TRUE for the time between sending PACKET_TURN_DONE and receiving
 * PACKET_NEW_YEAR.
 */
bool turn_done_sent = FALSE;

/**************************************************************************
  Convert a text string from the internal to the data encoding, when it
  is written to the network.
**************************************************************************/
static char *put_convert(const char *src, size_t *length)
{
  char *out = internal_to_data_string_malloc(src);

  if (out) {
    *length = strlen(out);
    return out;
  } else {
    *length = 0;
    return NULL;
  }
}

/**************************************************************************
  Convert a text string from the data to the internal encoding when it is
  first read from the network.  Returns FALSE if the destination isn't
  large enough or the source was bad.
**************************************************************************/
static bool get_convert(char *dst, size_t ndst,
                        const char *src, size_t nsrc)
{
  char *out = data_to_internal_string_malloc(src);
  bool ret = TRUE;
  size_t len;

  if (!out) {
    dst[0] = '\0';
    return FALSE;
  }

  len = strlen(out);
  if (ndst > 0 && len >= ndst) {
    ret = FALSE;
    len = ndst - 1;
  }

  memcpy(dst, out, len);
  dst[len] = '\0';
  free(out);

  return ret;
}

/**************************************************************************
  Set up charsets for the client.
**************************************************************************/
static void charsets_init(void)
{
  dio_set_put_conv_callback(put_convert);
  dio_set_get_conv_callback(get_convert);
}
/**************************************************************************
  ...
**************************************************************************/
static bool my_adns_input_ready_cb (int sock, int flags, void *data)
{
  adns_poll();
  return TRUE;
}
/**************************************************************************
  ...
**************************************************************************/
static bool my_adns_timer_cb (void *data)
{
  adns_check_expired();
  return TRUE;
}
/**************************************************************************
  ...
**************************************************************************/
static void my_init_adns(void)
{
  adns_init();
  if (adns_is_available()) {
    add_net_input_callback(adns_get_socket_fd(),
                           INPUT_READ,
                           my_adns_input_ready_cb,
                           NULL,
                           NULL);
    add_timer_callback(500000,
                       my_adns_timer_cb,
                       NULL);
  }
}

/**************************************************************************
 This is called at program exit.
**************************************************************************/
static void at_exit(void)
{
  client_kill_server(TRUE);
  my_shutdown_network();
}

/**************************************************************************
...
**************************************************************************/
int main(int argc, char *argv[])
{
  int i;
  int loglevel;
  int ui_options = 0;
  bool ui_separator = FALSE;
  char *option = NULL;
  char tileset_name[512] = "\0";
  char sound_plugin_name[512] = "\0";
  char sound_set_name[512] = "\0";
  char user_name[512] = "\0";
  char password[MAX_LEN_PASSWORD] = "\0";
  char metaserver[512] = "\0";

  /* Load win32 post-crash debugger */
#ifdef WIN32_NATIVE
# ifndef NDEBUG
  if (LoadLibrary("exchndl.dll") == NULL) {
#  ifdef DEBUG
    fprintf(stderr, "exchndl.dll could not be loaded, no crash debugger\n");
#  endif
  }
# endif
#endif

  init_nls();
  audio_init();

  /* default argument values are set in options.c */
  loglevel = LOG_NORMAL;
  //loglevel = LOG_DEBUG;

  i = 1;

  while (i < argc) {
    if (ui_separator) {
      argv[1 + ui_options] = argv[i];
      ui_options++;
    } else if (is_option("--help", argv[i])) {
      wc_fprintf(stderr, _("Usage: %s [option ...]\n"
                           "Valid options are:\n"), argv[0]);
      wc_fprintf(stderr, _("  -a, --autoconnect\tSkip connect dialog\n"));
#ifdef DEBUG
      wc_fprintf(stderr, _("  -d, --debug NUM\tSet debug log level (0 to 4,"
                           " or 4:file1,min,max:... or 4:file1:file2)\n"));
#else
      wc_fprintf(stderr,
                 _("  -d, --debug NUM\tSet debug log level (0 to 3)\n"));
#endif
      wc_fprintf(stderr,
                 _("  -h, --help\t\tPrint a summary of the options\n"));
      wc_fprintf(stderr,
                 _("  -l, --log FILE\tUse FILE as logfile "
                   "(spawned server also uses this)\n"));
      wc_fprintf(stderr,
                 _("  -m, --meta HOST\t"
                   "Connect to the metaserver at HOST\n"));
      wc_fprintf(stderr, _("  -n, --name NAME\tUse NAME as name\n"));
      wc_fprintf(stderr, _("  -N, --No-hack\t\tDo not send hack request\n"));
      wc_fprintf(stderr,
                 _("  -p, --port PORT\tConnect to server port PORT\n"));
      wc_fprintf(stderr, _("  -P, --Plugin PLUGIN\tUse PLUGIN for sound "
                           "output %s\n"),
                 audio_get_all_plugin_names());
      wc_fprintf(stderr,
                 _("  -r, --read FILE\tRead startup script FILE "
                   "(for spawned server only)\n"));
      wc_fprintf(stderr,
                 _("  -s, --server HOST\tConnect to the server at HOST\n"));
      wc_fprintf(stderr,
                 _("  -S, --Sound FILE\tRead sound tags from FILE\n"));
      wc_fprintf(stderr,
                 _("  -t, --tiles FILE\t"
                   "Use data file FILE.tilespec for tiles\n"));
      wc_fprintf(stderr,
                 _("  -v, --version\t\tPrint the version number\n"));
      wc_fprintf(stderr,
                 _("  -w, --word PASS\tUse PASS as "
                   "password\n"));
      wc_fprintf(stderr,
                 _("      --\t\t" "Pass any following options to the UI.\n"
                   "\t\t\tTry \"%s -- --help\" for more.\n"), argv[0]);
      exit(EXIT_SUCCESS);
    } else if (is_option("--version", argv[i])) {
      wc_fprintf(stderr, "%s %s %s client\n", warciv_name_version(),
                 warclient_name_version(), client_string);
      exit(EXIT_SUCCESS);
    } else if ((option = get_option("--log", argv, &i, argc))) {
      logfile = mystrdup(option); /* never free()d */
    } else if ((option = get_option("--read", argv, &i, argc))) {
      scriptfile = mystrdup(option);  /* never free()d */
    } else if ((option = get_option("--name", argv, &i, argc))) {
      sz_strlcpy(user_name, option);
    } else if (is_option("--No-hack", argv[i])) {
      do_not_request_hack = TRUE;
    } else if ((option = get_option("--meta", argv, &i, argc))) {
      sz_strlcpy(metaserver, option);
    } else if ((option = get_option("--Sound", argv, &i, argc))) {
      sz_strlcpy(sound_set_name, option);
    } else if ((option = get_option("--Plugin", argv, &i, argc))) {
      sz_strlcpy(sound_plugin_name, option);
    } else if ((option = get_option("--port", argv, &i, argc))) {
      if (sscanf(option, "%d", &server_port) != 1) {
        wc_fprintf(stderr, _("Invalid port \"%s\" specified with "
                             "--port option.\n"), option);
        wc_fprintf(stderr, _("Try using --help.\n"));
        exit(EXIT_FAILURE);
      }
    } else if ((option = get_option("--server", argv, &i, argc))) {
      sz_strlcpy(server_host, option);
    } else if (is_option("--autoconnect", argv[i])) {
      auto_connect = TRUE;
    } else if ((option = get_option("--debug", argv, &i, argc))) {
      loglevel = log_parse_level_str(option);
      if (loglevel == -1) {
        wc_fprintf(stderr,
                   _("Invalid debug level \"%s\" specified with --debug "
                     "option.\n"), option);
        wc_fprintf(stderr, _("Try using --help.\n"));
        exit(EXIT_FAILURE);
      }
    } else if ((option = get_option("--tiles", argv, &i, argc))) {
      sz_strlcpy(tileset_name, option);
    } else if ((option = get_option("--word", argv, &i, argc))) {
      sz_strlcpy(password, option);
    } else if (is_option("--", argv[i])) {
      ui_separator = TRUE;
    } else {
      wc_fprintf(stderr, _("Unrecognized option: \"%s\"\n"), argv[i]);
      exit(EXIT_FAILURE);
    }
    i++;
  }                             /* of while */

  /* Remove all options except those intended for the UI. */
  argv[1 + ui_options] = NULL;
  argc = 1 + ui_options;

  /* disallow running as root -- too dangerous */
  dont_run_as_root(argv[0], "warciv_client");

  log_init(logfile, loglevel, NULL);

  /* after log_init: */

  sz_strlcpy(default_user_name, user_username());
  if (!is_valid_username(default_user_name)) {
    char buf[sizeof(default_user_name)];

    my_snprintf(buf, sizeof(buf), "_%s", default_user_name);
    if (is_valid_username(buf)) {
      sz_strlcpy(default_user_name, buf);
    } else {
      my_snprintf(default_user_name, sizeof(default_user_name),
                  "player%d", myrand(10000));
    }
  }

  /* initialization */

  game.all_connections = connection_list_new();
  game.est_connections = connection_list_new();
  game.game_connections = connection_list_new();

  connection_init();
  charsets_init();
  my_init_network();
  my_init_adns();

  aconnection.used = FALSE;
  aconnection.established = FALSE;
  aconnection.player = NULL;

  /* register exit handler */
  atexit(at_exit);

  client_options_init();
  chatline_common_init();
  init_messages_where();
  init_city_report_data();
  init_player_dlg_common();
  settable_options_init();
  voteinfo_queue_init();

  load_general_options();

  if (tileset_name[0] != '\0') {
    sz_strlcpy(default_tileset_name, tileset_name);
  }
  if (sound_set_name[0] != '\0') {
    sz_strlcpy(default_sound_set_name, sound_set_name);
  }
  if (sound_plugin_name[0] != '\0') {
    sz_strlcpy(default_sound_plugin_name, sound_plugin_name);
  }
  if (server_host[0] == '\0') {
    sz_strlcpy(server_host, default_server_host);
  }
  if (user_name[0] != '\0') {
    sz_strlcpy(default_user_name, user_name);
  }
  if (password[0] != '\0') {
    sz_strlcpy(default_password, password);
  }
  if (metaserver[0] != '\0') {
    sz_strlcpy(default_metaserver, metaserver);
  }
  if (server_port == -1) {
    server_port = default_server_port;
  }


  /* This seed is not saved anywhere; randoms in the client should
     have cosmetic effects only (eg city name suggestions).  --dwp */
  mysrand(time(NULL));

  boot_help_texts();
  if (!tilespec_read_toplevel(default_tileset_name)) {
    /* get tile sizes etc */
    exit(EXIT_FAILURE);
  }

  audio_real_init();
  audio_play_music("music_start", NULL);

  /* run gui-specific client */
  ui_main(argc, argv);

  /* termination */
  ui_exit();

  /* not reached */
  return EXIT_SUCCESS;
}
/**************************************************************************
...
**************************************************************************/
void ui_exit(void)
{
  attribute_flush();
  client_remove_all_cli_conn();

  client_game_free();
  client_options_free();

  exit(EXIT_SUCCESS);
}


/**************************************************************************
...
**************************************************************************/
void handle_packet_input(void *packet, int opcode)
{
  if (!client_handle_packet(static_cast<packet_type>(opcode), packet)) {
    freelog(LOG_ERROR,
            "Received unknown packet (opcode %d) from server!",
            opcode);
  }
}

/**************************************************************************
...
**************************************************************************/
void user_ended_turn(void)
{
  send_turn_done();
}

/**************************************************************************
...
**************************************************************************/
void send_turn_done(void)
{
  freelog(LOG_DEBUG, "send_turn_done() turn_done_button_state=%d",
          get_turn_done_button_state());

  if (!get_turn_done_button_state()) {
    /*
     * The turn done button is disabled but the user may have press
     * the return key.
     */

    if (agents_busy()) {
      waiting_for_end_turn = TRUE;
    }

    return;
  }

  waiting_for_end_turn = FALSE;
  turn_done_sent = TRUE;

  attribute_flush();

  delayed_goto_event(AUTO_PRESS_TURN_DONE, NULL);
  send_packet_player_turn_done(&aconnection);

  update_turn_done_button_state();
}

/**************************************************************************
  ...
**************************************************************************/
void send_goto_unit(unit_t *punit, tile_t *dest_tile)
{
  dsend_packet_unit_goto(&aconnection, punit->id,
                         dest_tile->x, dest_tile->y);
}

/**************************************************************************
  ...
**************************************************************************/
void send_report_request(enum report_type type)
{
  dsend_packet_report_req(&aconnection, type);
}

/**************************************************************************
 called whenever client is changed to pre-game state.
**************************************************************************/
void client_game_init()
{
  freelog(LOG_DEBUG, "client_game_init");

  game_init();
  attribute_init();
  agents_init();
  city_autonaming_init();
  control_queues_init();
  link_marks_init();
  trade_init();
}

/**************************************************************************
...
**************************************************************************/
void client_game_free()
{
  free_mapview_updates();
  city_autonaming_free();
  control_queues_free();
  clear_all_link_marks();
  free_client_goto();
  free_help_texts();
  attribute_free();
  agents_free();
  trade_free();
  game_free();
}

/**************************************************************************
...
**************************************************************************/
void set_client_state(enum client_states newstate)
{
  bool connect_error = (client_state == CLIENT_PRE_GAME_STATE)
                       && newstate == CLIENT_PRE_GAME_STATE;
  enum client_states oldstate = client_state;
  player_t *pplayer = get_player_ptr();

  freelog(LOG_DEBUG, "set_client_state %d", newstate);

  if (newstate == CLIENT_GAME_OVER_STATE) {
    /*
     * Extra kludge for end-game handling of the CMA.
     */
    if (pplayer) {
      city_list_iterate(pplayer->cities, pcity) {
        if (cma_is_city_under_agent(pcity, NULL)) {
          cma_release_city(pcity);
        }
      } city_list_iterate_end;
    }
    popdown_all_city_dialogs();
    popdown_all_game_dialogs();
    set_unit_focus(NULL);
  }

  if (client_state != newstate) {

    /* If changing from pre-game state to _either_ select race
       or running state, then we have finished getting ruleset data,
       and should translate data, for joining running game or for
       selecting nations.  (Want translated nation names in nation
       select dialog.)
    */
    if (client_state == CLIENT_PRE_GAME_STATE
        && (newstate == CLIENT_SELECT_RACE_STATE
            || newstate == CLIENT_GAME_RUNNING_STATE)) {
      translate_data_names();
      audio_stop();             /* stop intro sound loop */
    }

    client_state = newstate;

    if (client_state == CLIENT_GAME_RUNNING_STATE) {
      player_colors_init();
      delayed_trade_routes_build();
      check_ruleset_specific_options();
      create_event(NULL, E_GAME_START, _("Game started."));
      precalc_tech_data();
      if (pplayer) {
        update_research(pplayer);
      }
      role_unit_precalcs();
      clear_notify_window();
      boot_help_texts();        /* reboot */
      can_slide = FALSE;
      update_unit_focus();
      can_slide = TRUE;
      set_client_page(PAGE_GAME);
      if (!client_is_observer()
          && pplayer
          && game.info.turn == 0)
      {
        set_default_user_tech_goal();
      }
      init_menus();

      /* Find something sensible to display instead of the intro gfx. */
      center_on_something();

      free_intro_radar_sprites();
      agents_game_start();

    } else if (client_state == CLIENT_PRE_GAME_STATE) {
      popdown_all_city_dialogs();
      popdown_all_game_dialogs();
      close_all_diplomacy_dialogs();
      set_unit_focus(NULL);
      clear_notify_window();
      if (oldstate != CLIENT_BOOT_STATE) {
        client_game_free();
      }
      client_game_init();
      if (!aconnection.established) {
        set_client_page(PAGE_MAIN);
      } else {
        set_client_page(PAGE_START);
      }
    }
  }

  if (!aconnection.established && client_state == CLIENT_PRE_GAME_STATE) {
    gui_server_connect();
    if (auto_connect) {
      if (connect_error) {
        freelog(LOG_NORMAL,
                _("There was an error while auto connecting; aborting."));
        exit(EXIT_FAILURE);
      } else {
        server_autoconnect();
        auto_connect = FALSE;   /* don't try this again */
      }
    }
  }

  update_turn_done_button_state();
  update_connection_list_dialog();
}


/**************************************************************************
...
**************************************************************************/
enum client_states get_client_state(void)
{
  return client_state;
}

/**************************************************************************
  Remove pconn from all connection lists in client, then free it.
**************************************************************************/
void client_remove_cli_conn(connection_t *pconn)
{
  if (pconn->player) {
    connection_list_unlink(pconn->player->connections, pconn);
  }
  connection_list_unlink(game.all_connections, pconn);
  connection_list_unlink(game.est_connections, pconn);
  connection_list_unlink(game.game_connections, pconn);
  assert(pconn != &aconnection);
  free(pconn);
}

/**************************************************************************
  Remove (and free) all connections from all connection lists in client.
  Assumes game.all_connections is properly maintained with all connections.
**************************************************************************/
void client_remove_all_cli_conn(void)
{
  while (connection_list_size(game.all_connections) > 0) {
    connection_t *pconn = connection_list_get(game.all_connections, 0);
    client_remove_cli_conn(pconn);
  }
}

void dealloc_id(int id); /* double kludge (suppress a possible warning) */
void dealloc_id(int id) { }/* kludge */

/**************************************************************************
..
**************************************************************************/
void send_attribute_block_request()
{
  send_packet_player_attribute_block(&aconnection);
}

/**************************************************************************
..
**************************************************************************/
void wait_till_request_got_processed(int request_id)
{
  input_from_server_till_request_got_processed(aconnection.sock,
                                               request_id);
}

/**************************************************************************
 This function should be called every 500ms. It lets the unit blink
 and update the timeout.
**************************************************************************/
void real_timer_callback(void)
{
  player_t *cplayer;
  time_t curtime;

  voteinfo_queue_check_removed();

  if (get_client_state() != CLIENT_GAME_RUNNING_STATE) {
    return;
  }

  cplayer = get_player_ptr();
  if (cplayer
      && cplayer->is_connected
      && cplayer->is_alive
      && !cplayer->turn_done) {
    int is_waiting = 0, is_moving = 0;

    players_iterate(pplayer) {
      if (pplayer->is_alive && pplayer->is_connected) {
        if (pplayer->turn_done) {
          is_waiting++;
        } else {
          is_moving++;
        }
      }
    } players_iterate_end;

    if (is_moving == 1 && is_waiting > 0) {
      update_turn_done_button(FALSE);   /* stress the slow player! */
    }
  }

  blink_active_unit();

  if (game.info.timeout > 0) {
    curtime = time(NULL);
    game.info.seconds_to_turndone = MAX(end_of_turn - curtime, 0);
  }
  delayed_goto_auto_timers_update();
  update_timeout_label();
}

/**************************************************************************
  Returns TRUE if the client can issue orders (such as giving unit
  commands).  This function should be called each time before allowing the
  user to give an order.
**************************************************************************/
bool can_client_issue_orders(void)
{
  return (!client_is_observer()
          && get_client_state() == CLIENT_GAME_RUNNING_STATE);
}

/**************************************************************************
  Returns TRUE iff the client can do diplomatic meetings with another
  given player.
**************************************************************************/
bool can_meet_with_player(player_t *pplayer)
{
  return (get_player_ptr()
          && could_meet_with_player(get_player_ptr(), pplayer)
          && can_client_issue_orders());
}

/**************************************************************************
  Returns TRUE iff the client can get intelligence from another
  given player.
**************************************************************************/
bool can_intel_with_player(player_t *pplayer)
{
  if (get_player_ptr()) {
    return could_intel_with_player(get_player_ptr(), pplayer);
  } else {
    /* Global observer */
    return client_is_observer();
  }
}

/**************************************************************************
  Return TRUE if the client can change the view; i.e. if the mapview is
  active.  This function should be called each time before allowing the
  user to do mapview actions.
**************************************************************************/
bool can_client_change_view(void)
{
  return (get_client_state() == CLIENT_GAME_RUNNING_STATE
          || get_client_state() == CLIENT_GAME_OVER_STATE);
}

/**************************************************************************
  Return the player that the client is controlling or observing.
**************************************************************************/
player_t *get_player_ptr(void)
{
  return aconnection.player;
}

/**************************************************************************
  Return the id of the player that the client is controlling or observing.
**************************************************************************/
unsigned int get_player_idx(void)
{
  return aconnection.player ? aconnection.player->player_no : -1;
}

/**************************************************************************
  ...
**************************************************************************/
bool client_is_observer(void)
{
  return aconnection.established && aconnection.observer;
}

/**************************************************************************
  Returns TRUE if and only if the client is controlling a player.
**************************************************************************/
bool client_is_player(void)
{
  return aconnection.established && conn_controls_player(&aconnection);
}

/**************************************************************************
  Returns TRUE if and only if the client is opbserving globally the game.
**************************************************************************/
bool client_is_global_observer(void)
{
  return aconnection.established && conn_is_global_observer(&aconnection);
}
