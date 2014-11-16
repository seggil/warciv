
 /****************************************************************************
 *                       THIS FILE WAS GENERATED                             *
 * Script: common/generate_packets.py                                        *
 * Input:  common/packets.def                                                *
 *                       DO NOT CHANGE THIS FILE                             *
 ****************************************************************************/



#ifdef HAVE_CONFIG_H
# include "../config.hh"
#endif
#include <stdio.h>

#include "packets.hh"
#include "log.hh"

#include "packhand_gen.hh"

bool client_handle_packet(enum packet_type type, void *packet)
{
  freelog(LOG_VERBOSE, "sc opcode=%d", type);
# if REPLAY
  printf("sc opc=%d ", type);
# endif
  switch(type) {
  case PACKET_PROCESSING_STARTED: /* 0 sc */
    handle_processing_started();  /* packhand */
    return TRUE;

  case PACKET_PROCESSING_FINISHED: /* 1 sc */
    handle_processing_finished();  /* packhand */
    return TRUE;

  case PACKET_FREEZE_HINT: /* 2 sc */
    handle_freeze_hint();  /* packhand */
    return TRUE;

  case PACKET_THAW_HINT: /* 3 sc */
    handle_thaw_hint();  /* packhand */
    return TRUE;

  /* PACKET_SERVER_JOIN_REQ 4 cs */

  case PACKET_SERVER_JOIN_REPLY: /* 5 sc */
    handle_server_join_reply(    /* packhand */
      ((struct packet_server_join_reply *)packet)->you_can_join,
      ((struct packet_server_join_reply *)packet)->message,
      ((struct packet_server_join_reply *)packet)->capability,
      ((struct packet_server_join_reply *)packet)->challenge_file,
      ((struct packet_server_join_reply *)packet)->conn_id);
    return TRUE;

  case PACKET_AUTHENTICATION_REQ: /* 6 sc */
    handle_authentication_req(    /* in client/gtk-gui-2.0/pages.c */
      ((struct packet_authentication_req *)packet)->type,
      ((struct packet_authentication_req *)packet)->message);
    return TRUE;

  /* PACKET_AUTHENTICATION_REPLY 7 cs */

  case PACKET_SERVER_SHUTDOWN: /* 8 sc */
    handle_server_shutdown();  /* packhand */
    return TRUE;

  case PACKET_NATION_UNAVAILABLE: /* 9 sc */
    handle_nation_unavailable(    /* packhand */
      ((struct packet_nation_unavailable *)packet)->nation);
    return TRUE;

  /* PACKET_NATION_SELECT_REQ 10;cs common/packets_gen.c */

  case PACKET_NATION_SELECT_OK: /* 11 sc */
    handle_nation_select_ok();  /* packhand */
    return TRUE;

  case PACKET_GAME_STATE: /* 12 sc */
    handle_game_state(    /* packhand */
      ((struct packet_game_state *)packet)->value);
    return TRUE;

  case PACKET_ENDGAME_REPORT:      /* 13 sc */
    handle_endgame_report((packet_endgame_report*)packet); /* packhand */
    return TRUE;

  case PACKET_TILE_INFO:      /* 14 sc */
    handle_tile_info((packet_tile_info*)packet); /* packhand */
    return TRUE;

  case PACKET_GAME_INFO:      /* 15 sc */
    handle_game_info((packet_game_info*)packet); /* packhand */
    return TRUE;

  case PACKET_MAP_INFO:      /* 16 sc */
    handle_map_info((packet_map_info*)packet); /* packhand */
    return TRUE;

  case PACKET_NUKE_TILE_INFO: /* 17 sc */
    handle_nuke_tile_info(    /* packhand */
      ((struct packet_nuke_tile_info *)packet)->x,
      ((struct packet_nuke_tile_info *)packet)->y);
    return TRUE;

  case PACKET_CHAT_MSG: /* 18 sc */
    handle_chat_msg(    /* packhand */
      ((struct packet_chat_msg *)packet)->message,
      ((struct packet_chat_msg *)packet)->x,
      ((struct packet_chat_msg *)packet)->y,
      ((struct packet_chat_msg *)packet)->event,
      ((struct packet_chat_msg *)packet)->conn_id);
    return TRUE;

  /* PACKET_CHAT_MSG_REQ 19;cs */

  case PACKET_CITY_REMOVE: /* 20 sc */
    handle_city_remove(    /* packhand */
      ((struct packet_city_remove *)packet)->city_id);
    return TRUE;

  case PACKET_CITY_INFO:      /* 21 sc */
    handle_city_info((packet_city_info*)packet); /* packhand */
    return TRUE;

  case PACKET_CITY_SHORT_INFO:      /* 22 sc */
    handle_city_short_info((packet_city_short_info*)packet); /* packhand */
    return TRUE;

  /* PACKET_CITY_SELL 23;cs */
  /* PACKET_CITY_BUY 24;cs */
  /* PACKET_CITY_CHANGE 25;cs */
  /* PACKET_CITY_WORKLIST 26;cs */
  /* PACKET_CITY_MAKE_SPECIALIST 27;cs */
  /* PACKET_CITY_MAKE_WORKER 28;cs */
  /* PACKET_CITY_CHANGE_SPECIALIST 29;cs */
  /* PACKET_CITY_RENAME 30;cs */
  /* PACKET_CITY_OPTIONS_REQ 31;cs */
  /* PACKET_CITY_REFRESH 32;cs */
  /* PACKET_CITY_INCITE_INQ 33;cs */

  case PACKET_CITY_INCITE_INFO: /* 34 sc */
    handle_city_incite_info(    /* packhand */
      ((struct packet_city_incite_info *)packet)->city_id,
      ((struct packet_city_incite_info *)packet)->cost);
    return TRUE;

  /* PACKET_CITY_NAME_SUGGESTION_REQ 35;cs */

  case PACKET_CITY_NAME_SUGGESTION_INFO: /* 36 sc */
    handle_city_name_suggestion_info(    /* packhand */
      ((struct packet_city_name_suggestion_info *)packet)->unit_id,
      ((struct packet_city_name_suggestion_info *)packet)->name);
    return TRUE;

  case PACKET_CITY_SABOTAGE_LIST: /* 37 sc */
    handle_city_sabotage_list(    /* packhand */
      ((struct packet_city_sabotage_list *)packet)->diplomat_id,
      ((struct packet_city_sabotage_list *)packet)->city_id,
      ((struct packet_city_sabotage_list *)packet)->improvements);
    return TRUE;

  case PACKET_PLAYER_REMOVE: /* 38 sc */
    handle_player_remove(    /* packhand */
      ((struct packet_player_remove *)packet)->player_id);
    return TRUE;

  case PACKET_PLAYER_INFO:      /* 39 sc */
    handle_player_info((packet_player_info*)packet); /* packhand */
    return TRUE;

  /* PACKET_PLAYER_TURN_DONE 40;cs */
  /* PACKET_PLAYER_RATES 41;cs */
  /* 42 */
  /* PACKET_PLAYER_CHANGE_GOVERNMENT 43;cs */
  /* PACKET_PLAYER_RESEARCH=44;cs */
  /* PACKET_PLAYER_TECH_GOAL=45;cs */
  /* PACKET_PLAYER_ATTRIBUTE_BLOCK=46;cs */

  case PACKET_PLAYER_ATTRIBUTE_CHUNK:      /* 47 cs, sc */
    handle_player_attribute_chunk((packet_player_attribute_chunk*)packet); /* packhand */
    return TRUE;

  case PACKET_UNIT_REMOVE: /* 48 sc */
    handle_unit_remove(    /* packhand */
      ((struct packet_unit_remove *)packet)->unit_id);
    return TRUE;

  case PACKET_UNIT_INFO:      /* 49 sc */
    handle_unit_info((packet_unit_info*)packet); /* packhand */
    return TRUE;

  case PACKET_UNIT_SHORT_INFO:      /* 50 sc */
    handle_unit_short_info((packet_unit_short_info*)packet); /* packhand */
    return TRUE;

  case PACKET_UNIT_COMBAT_INFO: /* 51 sc */
    handle_unit_combat_info(    /* packhand */
      ((struct packet_unit_combat_info *)packet)->attacker_unit_id,
      ((struct packet_unit_combat_info *)packet)->defender_unit_id,
      ((struct packet_unit_combat_info *)packet)->attacker_hp,
      ((struct packet_unit_combat_info *)packet)->defender_hp,
      ((struct packet_unit_combat_info *)packet)->make_winner_veteran);
    return TRUE;

  /* PACKET_UNIT_MOVE=52;cs */
  /* PACKET_UNIT_BUILD_CITY=53;cs */
  /* PACKET_UNIT_DISBAND=54;cs */
  /* PACKET_UNIT_CHANGE_HOMECITY=55;cs */
  /* PACKET_UNIT_ESTABLISH_TRADE=56;cs */
  /* PACKET_UNIT_HELP_BUILD_WONDER=57;cs */
  /* PACKET_UNIT_GOTO=58;cs */
  /* PACKET_UNIT_ORDERS=59;cs */
  /* PACKET_UNIT_AUTO=60;cs */
  /* PACKET_UNIT_UNLOAD=61;cs */
  /* PACKET_UNIT_UPGRADE=62;cs */
  /* PACKET_UNIT_NUKE=63;cs */
  /* PACKET_UNIT_PARADROP_TO=64;cs */
  /* PACKET_UNIT_AIRLIFT=65;cs */
  /* 66 */
  /* PACKET_UNIT_BRIBE_INQ=67;cs */

  case PACKET_UNIT_BRIBE_INFO: /* 68 sc */
    handle_unit_bribe_info(    /* packhand */
      ((struct packet_unit_bribe_info *)packet)->unit_id,
      ((struct packet_unit_bribe_info *)packet)->cost);
    return TRUE;

  /* PACKET_UNIT_TYPE_UPGRADE=69;cs */
  /* PACKET_UNIT_DIPLOMAT_ACTION=70;cs */

  case PACKET_UNIT_DIPLOMAT_POPUP_DIALOG: /* 71 sc */
    handle_unit_diplomat_popup_dialog(    /* packhand */
      ((struct packet_unit_diplomat_popup_dialog *)packet)->diplomat_id,
      ((struct packet_unit_diplomat_popup_dialog *)packet)->target_id);
    return TRUE;

  /* PACKET_UNIT_CHANGE_ACTIVITY=72; cs */
  /* PACKET_DIPLOMACY_INIT_MEETING_REQ=73;cs */

  case PACKET_DIPLOMACY_INIT_MEETING: /* 74 sc */
    handle_diplomacy_init_meeting(    /* client/gui-gtk-2.0/diplodlg.c */
      ((struct packet_diplomacy_init_meeting *)packet)->counterpart,
      ((struct packet_diplomacy_init_meeting *)packet)->initiated_from);
    return TRUE;

  /* PACKET_DIPLOMACY_CANCEL_MEETING_REQ=75;cs */

  case PACKET_DIPLOMACY_CANCEL_MEETING: /* 76 sc */
    handle_diplomacy_cancel_meeting(    /* client/gui-gtk-2.0/diplodlg.c */
      ((struct packet_diplomacy_cancel_meeting *)packet)->counterpart,
      ((struct packet_diplomacy_cancel_meeting *)packet)->initiated_from);
    return TRUE;

  /* PACKET_DIPLOMACY_CREATE_CLAUSE_REQ=77;cs */

  case PACKET_DIPLOMACY_CREATE_CLAUSE: /* 78 sc */
    handle_diplomacy_create_clause(    /* client/gui-gtk-2.0/diplodlg.c */
      ((struct packet_diplomacy_create_clause *)packet)->counterpart,
      ((struct packet_diplomacy_create_clause *)packet)->giver,
      ((struct packet_diplomacy_create_clause *)packet)->type,
      ((struct packet_diplomacy_create_clause *)packet)->value);
    return TRUE;

  /* PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ=79;cs */

  case PACKET_DIPLOMACY_REMOVE_CLAUSE: /* 80 sc */
    handle_diplomacy_remove_clause(    /* client/gui-gtk-2.0/diplodlg.c */
      ((struct packet_diplomacy_remove_clause *)packet)->counterpart,
      ((struct packet_diplomacy_remove_clause *)packet)->giver,
      ((struct packet_diplomacy_remove_clause *)packet)->type,
      ((struct packet_diplomacy_remove_clause *)packet)->value);
    return TRUE;

  /* PACKET_DIPLOMACY_ACCEPT_TREATY_REQ=81;cs */

  case PACKET_DIPLOMACY_ACCEPT_TREATY: /* 82 sc */
    handle_diplomacy_accept_treaty(    /* client/gui-gtk-2.0/diplodlg.c */
      ((struct packet_diplomacy_accept_treaty *)packet)->counterpart,
      ((struct packet_diplomacy_accept_treaty *)packet)->I_accepted,
      ((struct packet_diplomacy_accept_treaty *)packet)->other_accepted);
    return TRUE;

  /* PACKET_DIPLOMACY_CANCEL_PACT=83;cs */

  case PACKET_PAGE_MSG: /* 84 sc */
    handle_page_msg(    /* packhand */
      ((struct packet_page_msg *)packet)->message,
      ((struct packet_page_msg *)packet)->event);
    return TRUE;

  /* PACKET_REPORT_REQ=85;cs */

  case PACKET_CONN_INFO:      /* 86 sc */
    handle_conn_info((packet_conn_info*)packet); /* packhand */
    return TRUE;

  case PACKET_CONN_PING_INFO:      /* 87 sc */
    handle_conn_ping_info((packet_conn_ping_info*)packet); /* packhand */
    return TRUE;

  case PACKET_CONN_PING: /* 88 sc */
    handle_conn_ping();  /* packhand */
    return TRUE;

  /* PACKET_CONN_PONG=89;cs */

  case PACKET_BEFORE_NEW_YEAR: /* 90 sc */
    handle_before_new_year();  /* packhand */
    return TRUE;

  case PACKET_START_TURN: /* 91 sc */
    handle_start_turn();  /* packhand */
    return TRUE;

  case PACKET_NEW_YEAR: /* 92 sc */
    handle_new_year(    /* packhand */
      ((struct packet_new_year *)packet)->year,
      ((struct packet_new_year *)packet)->turn);
    return TRUE;

  /* PACKET_SPACESHIP_LAUNCH=93;cs */
  /* PACKET_SPACESHIP_PLACE=94;cs */

  case PACKET_SPACESHIP_INFO:      /* 95 sc */
    handle_spaceship_info((packet_spaceship_info*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_UNIT:      /* 96 sc */
    handle_ruleset_unit((packet_ruleset_unit*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_GAME:      /* 97 sc */
    handle_ruleset_game((packet_ruleset_game*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_GOVERNMENT_RULER_TITLE:      /* 98 sc */
    handle_ruleset_government_ruler_title((packet_ruleset_government_ruler_title*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_TECH:      /* 99 sc */
    handle_ruleset_tech((packet_ruleset_tech*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_GOVERNMENT:      /* 100 sc */
    handle_ruleset_government((packet_ruleset_government*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_TERRAIN_CONTROL:      /* 101 sc */
    handle_ruleset_terrain_control((packet_ruleset_terrain_control*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_NATION:      /* 102 sc */
    handle_ruleset_nation((packet_ruleset_nation*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_CITY:      /* 103 sc */
    handle_ruleset_city((packet_ruleset_city*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_BUILDING:      /* 104 sc */
    handle_ruleset_building((packet_ruleset_building*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_TERRAIN:      /* 105 sc */
    handle_ruleset_terrain((packet_ruleset_terrain*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_CONTROL:      /* 106 sc */
    handle_ruleset_control((packet_ruleset_control*)packet); /* packhand */
    return TRUE;

  /* PACKET_UNIT_LOAD=107;cs */
  /* PACKET_SINGLE_WANT_HACK_REQ=108;cs */

  case PACKET_SINGLE_WANT_HACK_REPLY: /* 109 sc */
    handle_single_want_hack_reply(    /* client/connectdlg_common.c */
      ((struct packet_single_want_hack_reply *)packet)->you_have_hack);
    return TRUE;

  /* 110 */

  case PACKET_GAME_LOAD:      /* 111 sc */
    handle_game_load((packet_game_load*)packet); /* gui-gtk-2.0/pages.c */
    return TRUE;

  case PACKET_OPTIONS_SETTABLE_CONTROL:      /* 112 sc */
    handle_options_settable_control((packet_options_settable_control*)packet); /* client/repodlgs_common.c */
    return TRUE;

  case PACKET_OPTIONS_SETTABLE:      /* 113 sc */
    handle_options_settable((packet_options_settable*)packet); /* client/repodlgs_common.c */
    return TRUE;

  case PACKET_SELECT_RACES: /* 114 sc */
    handle_select_races();  /* packhand */
    return TRUE;

/* 115 */
/* 116 */
/* 117 */
/* 118 */
/* 119 */

  case PACKET_RULESET_CACHE_GROUP:      /* 120 sc */
    handle_ruleset_cache_group((packet_ruleset_cache_group*)packet); /* packhand */
    return TRUE;

  case PACKET_RULESET_CACHE_EFFECT:      /* 121 sc */
    handle_ruleset_cache_effect((packet_ruleset_cache_effect*)packet); /* packhand */
    return TRUE;

  case PACKET_TRADEROUTE_INFO:      /* 122 sc */
    handle_traderoute_info((packet_traderoute_info*)packet); /* packhand */
    return TRUE;

  case PACKET_EXTGAME_INFO:      /* 123 sc */
    handle_extgame_info((packet_extgame_info*)packet); /* packhand */
    return TRUE;

  case PACKET_VOTE_NEW:      /* 124 sc */
    handle_vote_new((packet_vote_new*)packet); /* packhand */
    return TRUE;

  case PACKET_VOTE_UPDATE: /* 125 sc */
    handle_vote_update(    /* packhand */
      ((struct packet_vote_update *)packet)->vote_no,
      ((struct packet_vote_update *)packet)->yes,
      ((struct packet_vote_update *)packet)->no,
      ((struct packet_vote_update *)packet)->abstain,
      ((struct packet_vote_update *)packet)->num_voters);
    return TRUE;

  case PACKET_VOTE_REMOVE: /* 126 sc */
    handle_vote_remove(    /* packhand */
      ((struct packet_vote_remove *)packet)->vote_no);
    return TRUE;

  case PACKET_VOTE_RESOLVE: /* 127 sc */
    handle_vote_resolve(    /* packhand */
      ((struct packet_vote_resolve *)packet)->vote_no,
      ((struct packet_vote_resolve *)packet)->passed);
    return TRUE;

  /* PACKET_VOTE_SUBMIT=128;cs */
  /* 129 */
  /* PACKET_TRADE_ROUTE_PLAN=130;cs */

  case PACKET_TRADE_ROUTE_REMOVE: /* 131 cs sc */
    handle_trade_route_remove(    /* client/trade.c */
      ((struct packet_trade_route_remove *)packet)->city1,
      ((struct packet_trade_route_remove *)packet)->city2);
    return TRUE;

  /* PACKET_UNIT_TRADE_ROUTE=132 */

  case PACKET_TRADE_ROUTE_INFO:      /* 133 sc */
    handle_trade_route_info((packet_trade_route_info*)packet); /* client/trade.c */
    return TRUE;

  /* 134 */

  case PACKET_FREEZE_CLIENT: /* 135 sc */
    handle_freeze_client();  /* packhand */
    return TRUE;

  case PACKET_THAW_CLIENT: /* 136 sc */
    handle_thaw_client();  /* packhand */
    return TRUE;

  /* 137 */
  /* PACKET_CITY_SET_RALLY_POINT=138;cs */
  /* PACKET_CITY_CLEAR_RALLY_POINT=139;cs */
  /* 140 */
  /* PACKET_UNIT_AIR_PATROL=141;cs */
  /* PACKET_UNIT_AIR_PATROL_STOP=142;cs */
  /* 143 */
  /* 144 */

  case PACKET_CITY_MANAGER_PARAM:      /* 145 cs sc */
    handle_city_manager_param((packet_city_manager_param*)packet); /* packhand */
    return TRUE;

  case PACKET_CITY_NO_MANAGER_PARAM: /* 146 cs sc */
    handle_city_no_manager_param(    /* packhand */
      ((struct packet_city_no_manager_param *)packet)->id);
    return TRUE;

  /* 147 */
  /* 148 */
  /* 149 */
  /* PACKET_PLAYER_INFO_REQ=150;cs */

  default:
    return FALSE;
  }
}
