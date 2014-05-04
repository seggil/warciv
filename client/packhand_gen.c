
 /****************************************************************************
 *                       THIS FILE WAS GENERATED                             *
 * Script: common/generate_packets.py                                        *
 * Input:  common/packets.def                                                *
 *                       DO NOT CHANGE THIS FILE                             *
 ****************************************************************************/



#ifdef HAVE_CONFIG_H
# include "../config.h"
#endif
#include <stdio.h>

#include "packets.h"
#include "log.h"

#include "packhand_gen.h"

bool client_handle_packet(enum packet_type type, void *packet)
{
  freelog(LOG_VERBOSE, "sc opcode=%d", type);
# if REPLAY
  printf("sc opcode=%d ", type);
# endif
  switch(type) {
  case PACKET_PROCESSING_STARTED: /* 0 */
    handle_processing_started();
    return TRUE;

  case PACKET_PROCESSING_FINISHED: /* 1 */
    handle_processing_finished();
    return TRUE;

  case PACKET_FREEZE_HINT: /* 2 */
    handle_freeze_hint();
    return TRUE;

  case PACKET_THAW_HINT: /* 3 */
    handle_thaw_hint();
    return TRUE;

  /* PACKET_SERVER_JOIN_REQ 4 */

  case PACKET_SERVER_JOIN_REPLY: /* 5 */
    handle_server_join_reply(
      ((struct packet_server_join_reply *)packet)->you_can_join,
      ((struct packet_server_join_reply *)packet)->message,
      ((struct packet_server_join_reply *)packet)->capability,
      ((struct packet_server_join_reply *)packet)->challenge_file,
      ((struct packet_server_join_reply *)packet)->conn_id);
    return TRUE;

  case PACKET_AUTHENTICATION_REQ: /* 6 */
    handle_authentication_req( /* in client/gtk-gui-2.0/pages.c */
      ((struct packet_authentication_req *)packet)->type,
      ((struct packet_authentication_req *)packet)->message);
    return TRUE;

  /* PACKET_AUTHENTICATION_REPLY 7 */

  case PACKET_SERVER_SHUTDOWN: /* 8 */
    handle_server_shutdown();
    return TRUE;

  case PACKET_NATION_UNAVAILABLE: /* 9 */
    handle_nation_unavailable(
      ((struct packet_nation_unavailable *)packet)->nation);
    return TRUE;

  /* PACKET_NATION_SELECT_REQ 10;cs */

  case PACKET_NATION_SELECT_OK: /* 11 */
    handle_nation_select_ok();
    return TRUE;

  case PACKET_GAME_STATE: /* 12 */
    handle_game_state(
      ((struct packet_game_state *)packet)->value);
    return TRUE;

  case PACKET_ENDGAME_REPORT: /* 13 */
    handle_endgame_report(packet);
    return TRUE;

  case PACKET_TILE_INFO: /* 14 */
    handle_tile_info(packet);
    return TRUE;

  case PACKET_GAME_INFO: /* 15 */
    handle_game_info(packet);
    return TRUE;

  case PACKET_MAP_INFO: /* 16 */
    handle_map_info(packet);
    return TRUE;

  case PACKET_NUKE_TILE_INFO: /* 17 */
    handle_nuke_tile_info(
      ((struct packet_nuke_tile_info *)packet)->x,
      ((struct packet_nuke_tile_info *)packet)->y);
    return TRUE;

  case PACKET_CHAT_MSG: /* 18 */
    handle_chat_msg(
      ((struct packet_chat_msg *)packet)->message,
      ((struct packet_chat_msg *)packet)->x,
      ((struct packet_chat_msg *)packet)->y,
      ((struct packet_chat_msg *)packet)->event,
      ((struct packet_chat_msg *)packet)->conn_id);
    return TRUE;

  /* PACKET_CHAT_MSG_REQ 19;cs */

  case PACKET_CITY_REMOVE: /* 20 */
    handle_city_remove(
      ((struct packet_city_remove *)packet)->city_id);
    return TRUE;

  case PACKET_CITY_INFO: /* 21 */
    handle_city_info(packet);
    return TRUE;

  case PACKET_CITY_SHORT_INFO: /* 22 */
    handle_city_short_info(packet);
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

  case PACKET_CITY_INCITE_INFO: /* 34 */
    handle_city_incite_info(
      ((struct packet_city_incite_info *)packet)->city_id,
      ((struct packet_city_incite_info *)packet)->cost);
    return TRUE;

  /* PACKET_CITY_NAME_SUGGESTION_REQ 35;cs */

  case PACKET_CITY_NAME_SUGGESTION_INFO: /* 36 */
    handle_city_name_suggestion_info(
      ((struct packet_city_name_suggestion_info *)packet)->unit_id,
      ((struct packet_city_name_suggestion_info *)packet)->name);
    return TRUE;

  case PACKET_CITY_SABOTAGE_LIST: /* 37 */
    handle_city_sabotage_list(
      ((struct packet_city_sabotage_list *)packet)->diplomat_id,
      ((struct packet_city_sabotage_list *)packet)->city_id,
      ((struct packet_city_sabotage_list *)packet)->improvements);
    return TRUE;

  case PACKET_PLAYER_REMOVE: /* 38 */
    handle_player_remove(
      ((struct packet_player_remove *)packet)->player_id);
    return TRUE;

  case PACKET_PLAYER_INFO: /* 39 */
    handle_player_info(packet);
    return TRUE;

  /* PACKET_PLAYER_TURN_DONE 40;cs */
  /* PACKET_PLAYER_RATES 41;cs */
  /* 42 */
  /* PACKET_PLAYER_CHANGE_GOVERNMENT 43;cs */
  /* PACKET_PLAYER_RESEARCH=44;cs */
  /* PACKET_PLAYER_TECH_GOAL=45;cs */
  /* PACKET_PLAYER_ATTRIBUTE_BLOCK=46;cs */

  case PACKET_PLAYER_ATTRIBUTE_CHUNK: /* 47 */
    handle_player_attribute_chunk(packet);
    return TRUE;

  case PACKET_UNIT_REMOVE: /* 48 */
    handle_unit_remove(
      ((struct packet_unit_remove *)packet)->unit_id);
    return TRUE;

  case PACKET_UNIT_INFO: /* 49 */
    handle_unit_info(packet);
    return TRUE;

  case PACKET_UNIT_SHORT_INFO: /* 50 */
    handle_unit_short_info(packet);
    return TRUE;

  case PACKET_UNIT_COMBAT_INFO: /* 51 */
    handle_unit_combat_info(
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

  case PACKET_UNIT_BRIBE_INFO: /* 68 */
    handle_unit_bribe_info(
      ((struct packet_unit_bribe_info *)packet)->unit_id,
      ((struct packet_unit_bribe_info *)packet)->cost);
    return TRUE;

  /* PACKET_UNIT_TYPE_UPGRADE=69;cs */
  /* PACKET_UNIT_DIPLOMAT_ACTION=70;cs */

  case PACKET_UNIT_DIPLOMAT_POPUP_DIALOG: /* 71 */
    handle_unit_diplomat_popup_dialog(
      ((struct packet_unit_diplomat_popup_dialog *)packet)->diplomat_id,
      ((struct packet_unit_diplomat_popup_dialog *)packet)->target_id);
    return TRUE;

  /* PACKET_UNIT_CHANGE_ACTIVITY=72; cs */
  /* PACKET_DIPLOMACY_INIT_MEETING_REQ=73;cs */

  case PACKET_DIPLOMACY_INIT_MEETING: /* 74 */
    handle_diplomacy_init_meeting(
      ((struct packet_diplomacy_init_meeting *)packet)->counterpart,
      ((struct packet_diplomacy_init_meeting *)packet)->initiated_from);
    return TRUE;

  /* PACKET_DIPLOMACY_CANCEL_MEETING_REQ=75;cs */

  case PACKET_DIPLOMACY_CANCEL_MEETING: /* 76 */
    handle_diplomacy_cancel_meeting(
      ((struct packet_diplomacy_cancel_meeting *)packet)->counterpart,
      ((struct packet_diplomacy_cancel_meeting *)packet)->initiated_from);
    return TRUE;

  /* PACKET_DIPLOMACY_CREATE_CLAUSE_REQ=77;cs */

  case PACKET_DIPLOMACY_CREATE_CLAUSE: /* 78 */
    handle_diplomacy_create_clause(
      ((struct packet_diplomacy_create_clause *)packet)->counterpart,
      ((struct packet_diplomacy_create_clause *)packet)->giver,
      ((struct packet_diplomacy_create_clause *)packet)->type,
      ((struct packet_diplomacy_create_clause *)packet)->value);
    return TRUE;

  /* PACKET_DIPLOMACY_REMOVE_CLAUSE_REQ=79;cs */

  case PACKET_DIPLOMACY_REMOVE_CLAUSE: /* 80 */
    handle_diplomacy_remove_clause(
      ((struct packet_diplomacy_remove_clause *)packet)->counterpart,
      ((struct packet_diplomacy_remove_clause *)packet)->giver,
      ((struct packet_diplomacy_remove_clause *)packet)->type,
      ((struct packet_diplomacy_remove_clause *)packet)->value);
    return TRUE;

  /* PACKET_DIPLOMACY_ACCEPT_TREATY_REQ=81;cs */

  case PACKET_DIPLOMACY_ACCEPT_TREATY: /* 82 */
    handle_diplomacy_accept_treaty(
      ((struct packet_diplomacy_accept_treaty *)packet)->counterpart,
      ((struct packet_diplomacy_accept_treaty *)packet)->I_accepted,
      ((struct packet_diplomacy_accept_treaty *)packet)->other_accepted);
    return TRUE;

  /* PACKET_DIPLOMACY_CANCEL_PACT=83;cs */

  case PACKET_PAGE_MSG: /* 84 */
    handle_page_msg(
      ((struct packet_page_msg *)packet)->message,
      ((struct packet_page_msg *)packet)->event);
    return TRUE;

  /* PACKET_REPORT_REQ=85;cs */

  case PACKET_CONN_INFO: /* 86 */
    handle_conn_info(packet);
    return TRUE;

  case PACKET_CONN_PING_INFO: /* 87 */
    handle_conn_ping_info(packet);
    return TRUE;

  case PACKET_CONN_PING: /* 88 */
    handle_conn_ping();
    return TRUE;

  /* PACKET_CONN_PONG=89;cs */

  case PACKET_BEFORE_NEW_YEAR: /* 90 */
    handle_before_new_year();
    return TRUE;

  case PACKET_START_TURN: /* 91 */
    handle_start_turn();
    return TRUE;

  case PACKET_NEW_YEAR: /* 92 */
    handle_new_year(
      ((struct packet_new_year *)packet)->year,
      ((struct packet_new_year *)packet)->turn);
    return TRUE;

  /* PACKET_SPACESHIP_LAUNCH=93;cs */
  /* PACKET_SPACESHIP_PLACE=94;cs */

  case PACKET_SPACESHIP_INFO: /* 95 */
    handle_spaceship_info(packet);
    return TRUE;

  case PACKET_RULESET_UNIT: /* 96 */
    handle_ruleset_unit(packet);
    return TRUE;

  case PACKET_RULESET_GAME: /* 97 */
    handle_ruleset_game(packet);
    return TRUE;

  case PACKET_RULESET_GOVERNMENT_RULER_TITLE: /* 98 */
    handle_ruleset_government_ruler_title(packet);
    return TRUE;

  case PACKET_RULESET_TECH: /* 99 */
    handle_ruleset_tech(packet);
    return TRUE;

  case PACKET_RULESET_GOVERNMENT: /* 100 */
    handle_ruleset_government(packet);
    return TRUE;

  case PACKET_RULESET_TERRAIN_CONTROL: /* 101 */
    handle_ruleset_terrain_control(packet);
    return TRUE;

  case PACKET_RULESET_NATION: /* 102 */
    handle_ruleset_nation(packet);
    return TRUE;

  case PACKET_RULESET_CITY: /* 103 */
    handle_ruleset_city(packet);
    return TRUE;

  case PACKET_RULESET_BUILDING: /* 104 */
    handle_ruleset_building(packet);
    return TRUE;

  case PACKET_RULESET_TERRAIN: /* 105 */
    handle_ruleset_terrain(packet);
    return TRUE;

  case PACKET_RULESET_CONTROL: /* 106 */
    handle_ruleset_control(packet);
    return TRUE;

  /* PACKET_UNIT_LOAD=107;cs */
  /* PACKET_SINGLE_WANT_HACK_REQ=108;cs */

  case PACKET_SINGLE_WANT_HACK_REPLY: /* 109 */
    handle_single_want_hack_reply(
      ((struct packet_single_want_hack_reply *)packet)->you_have_hack);
    return TRUE;

  /* 110 */

  case PACKET_GAME_LOAD: /* 111 */
    handle_game_load(packet);
    return TRUE;

  case PACKET_OPTIONS_SETTABLE_CONTROL: /* 112 */
    handle_options_settable_control(packet);
    return TRUE;

  case PACKET_OPTIONS_SETTABLE: /* 113 */
    handle_options_settable(packet);
    return TRUE;

  case PACKET_SELECT_RACES: /* 114 */
    handle_select_races();
    return TRUE;

/* 115 */
/* 116 */
/* 117 */
/* 118 */
/* 119 */

  case PACKET_RULESET_CACHE_GROUP: /* 120 */
    handle_ruleset_cache_group(packet);
    return TRUE;

  case PACKET_RULESET_CACHE_EFFECT: /* 121 */
    handle_ruleset_cache_effect(packet);
    return TRUE;

  case PACKET_TRADEROUTE_INFO: /* 122 */
    handle_traderoute_info(packet);
    return TRUE;

  case PACKET_EXTGAME_INFO: /* 123 */
    handle_extgame_info(packet);
    return TRUE;

  case PACKET_VOTE_NEW: /* 124 */
    handle_vote_new(packet);
    return TRUE;

  case PACKET_VOTE_UPDATE: /* 125 */
    handle_vote_update(
      ((struct packet_vote_update *)packet)->vote_no,
      ((struct packet_vote_update *)packet)->yes,
      ((struct packet_vote_update *)packet)->no,
      ((struct packet_vote_update *)packet)->abstain,
      ((struct packet_vote_update *)packet)->num_voters);
    return TRUE;

  case PACKET_VOTE_REMOVE: /* 126 */
    handle_vote_remove(
      ((struct packet_vote_remove *)packet)->vote_no);
    return TRUE;

  case PACKET_VOTE_RESOLVE: /* 127 */
    handle_vote_resolve(
      ((struct packet_vote_resolve *)packet)->vote_no,
      ((struct packet_vote_resolve *)packet)->passed);
    return TRUE;

  /* PACKET_VOTE_SUBMIT=128;cs */
  /* 129 */
  /* PACKET_TRADE_ROUTE_PLAN=130;cs */

  case PACKET_TRADE_ROUTE_REMOVE: /* 131 */
    handle_trade_route_remove(
      ((struct packet_trade_route_remove *)packet)->city1,
      ((struct packet_trade_route_remove *)packet)->city2);
    return TRUE;

  /* PACKET_UNIT_TRADE_ROUTE=132 */

  case PACKET_TRADE_ROUTE_INFO: /* 133 */
    handle_trade_route_info(packet);
    return TRUE;

  case PACKET_FREEZE_CLIENT: /* 135 */
    handle_freeze_client();
    return TRUE;

  case PACKET_THAW_CLIENT: /* 136 */
    handle_thaw_client();
    return TRUE;

  /* 137 */
  /* PACKET_CITY_SET_RALLY_POINT=138;cs */
  /* PACKET_CITY_CLEAR_RALLY_POINT=139;cs */
  /* 140 */
  /* PACKET_UNIT_AIR_PATROL=141;cs */
  /* PACKET_UNIT_AIR_PATROL_STOP=142;cs */
  /* 143 */
  /* 144 */

  case PACKET_CITY_MANAGER_PARAM: /* 145 */
    handle_city_manager_param(packet);
    return TRUE;

  case PACKET_CITY_NO_MANAGER_PARAM: /* 146 */
    handle_city_no_manager_param(
      ((struct packet_city_no_manager_param *)packet)->id);
    return TRUE;

  /* PACKET_PLAYER_INFO_REQ=150;cs */

  default:
    return FALSE;
  }
}
