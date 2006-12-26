
 /****************************************************************************
 *                       THIS FILE WAS GENERATED                             *
 * Script: common/generate_packets.py                                        *
 * Input:  common/packets.def                                                *
 *                       DO NOT CHANGE THIS FILE                             *
 ****************************************************************************/



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "log.h"
#include "packets.h"

#include "packhand_gen.h"
    
bool client_handle_packet(enum packet_type type, void *packet)
{
  switch(type) {
  case PACKET_PROCESSING_STARTED:
    freelog(LOG_VERBOSE, "handle_processing_started");
    handle_processing_started();
    return TRUE;

  case PACKET_PROCESSING_FINISHED:
    freelog(LOG_VERBOSE, "handle_processing_finished");
    handle_processing_finished();
    return TRUE;

  case PACKET_FREEZE_HINT:
    freelog(LOG_VERBOSE, "handle_freeze_hint");
    handle_freeze_hint();
    return TRUE;

  case PACKET_THAW_HINT:
    freelog(LOG_VERBOSE, "handle_thaw_hint");
    handle_thaw_hint();
    return TRUE;

  case PACKET_SERVER_JOIN_REPLY:
    freelog(LOG_VERBOSE, "handle_server_join_reply");
    handle_server_join_reply(
      ((struct packet_server_join_reply *)packet)->you_can_join,
      ((struct packet_server_join_reply *)packet)->message,
      ((struct packet_server_join_reply *)packet)->capability,
      ((struct packet_server_join_reply *)packet)->challenge_file,
      ((struct packet_server_join_reply *)packet)->conn_id);
    return TRUE;

  case PACKET_AUTHENTICATION_REQ:
    freelog(LOG_VERBOSE, "handle_authentication_req");
    handle_authentication_req(
      ((struct packet_authentication_req *)packet)->type,
      ((struct packet_authentication_req *)packet)->message);
    return TRUE;

  case PACKET_SERVER_SHUTDOWN:
    freelog(LOG_VERBOSE, "handle_server_shutdown");
    handle_server_shutdown();
    return TRUE;

  case PACKET_NATION_UNAVAILABLE:
    freelog(LOG_VERBOSE, "handle_nation_unavailable");
    handle_nation_unavailable(
      ((struct packet_nation_unavailable *)packet)->nation);
    return TRUE;

  case PACKET_SELECT_RACES:
    freelog(LOG_VERBOSE, "handle_select_races");
    handle_select_races();
    return TRUE;

  case PACKET_NATION_SELECT_OK:
    freelog(LOG_VERBOSE, "handle_nation_select_ok");
    handle_nation_select_ok();
    return TRUE;

  case PACKET_GAME_STATE:
    freelog(LOG_VERBOSE, "handle_game_state");
    handle_game_state(
      ((struct packet_game_state *)packet)->value);
    return TRUE;

  case PACKET_ENDGAME_REPORT:
    freelog(LOG_VERBOSE, "handle_endgame_report");
    handle_endgame_report(packet);
    return TRUE;

  case PACKET_TILE_INFO:
    freelog(LOG_VERBOSE, "handle_tile_info");
    handle_tile_info(packet);
    return TRUE;

  case PACKET_GAME_INFO:
    freelog(LOG_VERBOSE, "handle_game_info");
    handle_game_info(packet);
    return TRUE;

  case PACKET_MAP_INFO:
    freelog(LOG_VERBOSE, "handle_map_info");
    handle_map_info(
      ((struct packet_map_info *)packet)->xsize,
      ((struct packet_map_info *)packet)->ysize,
      ((struct packet_map_info *)packet)->topology_id);
    return TRUE;

  case PACKET_NUKE_TILE_INFO:
    freelog(LOG_VERBOSE, "handle_nuke_tile_info");
    handle_nuke_tile_info(
      ((struct packet_nuke_tile_info *)packet)->x,
      ((struct packet_nuke_tile_info *)packet)->y);
    return TRUE;

  case PACKET_CHAT_MSG:
    freelog(LOG_VERBOSE, "handle_chat_msg");
    handle_chat_msg(
      ((struct packet_chat_msg *)packet)->message,
      ((struct packet_chat_msg *)packet)->x,
      ((struct packet_chat_msg *)packet)->y,
      ((struct packet_chat_msg *)packet)->event,
      ((struct packet_chat_msg *)packet)->conn_id);
    return TRUE;

  case PACKET_CITY_REMOVE:
    freelog(LOG_VERBOSE, "handle_city_remove");
    handle_city_remove(
      ((struct packet_city_remove *)packet)->city_id);
    return TRUE;

  case PACKET_CITY_INFO:
    freelog(LOG_VERBOSE, "handle_city_info");
    handle_city_info(packet);
    return TRUE;

  case PACKET_CITY_SHORT_INFO:
    freelog(LOG_VERBOSE, "handle_city_short_info");
    handle_city_short_info(packet);
    return TRUE;

  case PACKET_CITY_INCITE_INFO:
    freelog(LOG_VERBOSE, "handle_city_incite_info");
    handle_city_incite_info(
      ((struct packet_city_incite_info *)packet)->city_id,
      ((struct packet_city_incite_info *)packet)->cost);
    return TRUE;

  case PACKET_CITY_NAME_SUGGESTION_INFO:
    freelog(LOG_VERBOSE, "handle_city_name_suggestion_info");
    handle_city_name_suggestion_info(
      ((struct packet_city_name_suggestion_info *)packet)->unit_id,
      ((struct packet_city_name_suggestion_info *)packet)->name);
    return TRUE;

  case PACKET_CITY_SABOTAGE_LIST:
    freelog(LOG_VERBOSE, "handle_city_sabotage_list");
    handle_city_sabotage_list(
      ((struct packet_city_sabotage_list *)packet)->diplomat_id,
      ((struct packet_city_sabotage_list *)packet)->city_id,
      ((struct packet_city_sabotage_list *)packet)->improvements);
    return TRUE;

  case PACKET_PLAYER_REMOVE:
    freelog(LOG_VERBOSE, "handle_player_remove");
    handle_player_remove(
      ((struct packet_player_remove *)packet)->player_id);
    return TRUE;

  case PACKET_PLAYER_INFO:
    freelog(LOG_VERBOSE, "handle_player_info");
    handle_player_info(packet);
    return TRUE;

  case PACKET_PLAYER_ATTRIBUTE_CHUNK:
    freelog(LOG_VERBOSE, "handle_player_attribute_chunk");
    handle_player_attribute_chunk(packet);
    return TRUE;

  case PACKET_UNIT_REMOVE:
    freelog(LOG_VERBOSE, "handle_unit_remove");
    handle_unit_remove(
      ((struct packet_unit_remove *)packet)->unit_id);
    return TRUE;

  case PACKET_UNIT_INFO:
    freelog(LOG_VERBOSE, "handle_unit_info");
    handle_unit_info(packet);
    return TRUE;

  case PACKET_UNIT_SHORT_INFO:
    freelog(LOG_VERBOSE, "handle_unit_short_info");
    handle_unit_short_info(packet);
    return TRUE;

  case PACKET_UNIT_COMBAT_INFO:
    freelog(LOG_VERBOSE, "handle_unit_combat_info");
    handle_unit_combat_info(
      ((struct packet_unit_combat_info *)packet)->attacker_unit_id,
      ((struct packet_unit_combat_info *)packet)->defender_unit_id,
      ((struct packet_unit_combat_info *)packet)->attacker_hp,
      ((struct packet_unit_combat_info *)packet)->defender_hp,
      ((struct packet_unit_combat_info *)packet)->make_winner_veteran);
    return TRUE;

  case PACKET_UNIT_BRIBE_INFO:
    freelog(LOG_VERBOSE, "handle_unit_bribe_info");
    handle_unit_bribe_info(
      ((struct packet_unit_bribe_info *)packet)->unit_id,
      ((struct packet_unit_bribe_info *)packet)->cost);
    return TRUE;

  case PACKET_UNIT_DIPLOMAT_POPUP_DIALOG:
    freelog(LOG_VERBOSE, "handle_unit_diplomat_popup_dialog");
    handle_unit_diplomat_popup_dialog(
      ((struct packet_unit_diplomat_popup_dialog *)packet)->diplomat_id,
      ((struct packet_unit_diplomat_popup_dialog *)packet)->target_id);
    return TRUE;

  case PACKET_DIPLOMACY_INIT_MEETING:
    freelog(LOG_VERBOSE, "handle_diplomacy_init_meeting");
    handle_diplomacy_init_meeting(
      ((struct packet_diplomacy_init_meeting *)packet)->counterpart,
      ((struct packet_diplomacy_init_meeting *)packet)->initiated_from);
    return TRUE;

  case PACKET_DIPLOMACY_CANCEL_MEETING:
    freelog(LOG_VERBOSE, "handle_diplomacy_cancel_meeting");
    handle_diplomacy_cancel_meeting(
      ((struct packet_diplomacy_cancel_meeting *)packet)->counterpart,
      ((struct packet_diplomacy_cancel_meeting *)packet)->initiated_from);
    return TRUE;

  case PACKET_DIPLOMACY_CREATE_CLAUSE:
    freelog(LOG_VERBOSE, "handle_diplomacy_create_clause");
    handle_diplomacy_create_clause(
      ((struct packet_diplomacy_create_clause *)packet)->counterpart,
      ((struct packet_diplomacy_create_clause *)packet)->giver,
      ((struct packet_diplomacy_create_clause *)packet)->type,
      ((struct packet_diplomacy_create_clause *)packet)->value);
    return TRUE;

  case PACKET_DIPLOMACY_REMOVE_CLAUSE:
    freelog(LOG_VERBOSE, "handle_diplomacy_remove_clause");
    handle_diplomacy_remove_clause(
      ((struct packet_diplomacy_remove_clause *)packet)->counterpart,
      ((struct packet_diplomacy_remove_clause *)packet)->giver,
      ((struct packet_diplomacy_remove_clause *)packet)->type,
      ((struct packet_diplomacy_remove_clause *)packet)->value);
    return TRUE;

  case PACKET_DIPLOMACY_ACCEPT_TREATY:
    freelog(LOG_VERBOSE, "handle_diplomacy_accept_treaty");
    handle_diplomacy_accept_treaty(
      ((struct packet_diplomacy_accept_treaty *)packet)->counterpart,
      ((struct packet_diplomacy_accept_treaty *)packet)->I_accepted,
      ((struct packet_diplomacy_accept_treaty *)packet)->other_accepted);
    return TRUE;

  case PACKET_PAGE_MSG:
    freelog(LOG_VERBOSE, "handle_page_msg");
    handle_page_msg(
      ((struct packet_page_msg *)packet)->message,
      ((struct packet_page_msg *)packet)->event);
    return TRUE;

  case PACKET_CONN_INFO:
    freelog(LOG_VERBOSE, "handle_conn_info");
    handle_conn_info(packet);
    return TRUE;

  case PACKET_CONN_PING_INFO:
    freelog(LOG_VERBOSE, "handle_conn_ping_info");
    handle_conn_ping_info(packet);
    return TRUE;

  case PACKET_CONN_PING:
    freelog(LOG_VERBOSE, "handle_conn_ping");
    handle_conn_ping();
    return TRUE;

  case PACKET_BEFORE_NEW_YEAR:
    freelog(LOG_VERBOSE, "handle_before_new_year");
    handle_before_new_year();
    return TRUE;

  case PACKET_START_TURN:
    freelog(LOG_VERBOSE, "handle_start_turn");
    handle_start_turn();
    return TRUE;

  case PACKET_NEW_YEAR:
    freelog(LOG_VERBOSE, "handle_new_year");
    handle_new_year(
      ((struct packet_new_year *)packet)->year,
      ((struct packet_new_year *)packet)->turn);
    return TRUE;

  case PACKET_SPACESHIP_INFO:
    freelog(LOG_VERBOSE, "handle_spaceship_info");
    handle_spaceship_info(packet);
    return TRUE;

  case PACKET_RULESET_UNIT:
    freelog(LOG_VERBOSE, "handle_ruleset_unit");
    handle_ruleset_unit(packet);
    return TRUE;

  case PACKET_RULESET_GAME:
    freelog(LOG_VERBOSE, "handle_ruleset_game");
    handle_ruleset_game(packet);
    return TRUE;

  case PACKET_RULESET_GOVERNMENT_RULER_TITLE:
    freelog(LOG_VERBOSE, "handle_ruleset_government_ruler_title");
    handle_ruleset_government_ruler_title(packet);
    return TRUE;

  case PACKET_RULESET_TECH:
    freelog(LOG_VERBOSE, "handle_ruleset_tech");
    handle_ruleset_tech(packet);
    return TRUE;

  case PACKET_RULESET_GOVERNMENT:
    freelog(LOG_VERBOSE, "handle_ruleset_government");
    handle_ruleset_government(packet);
    return TRUE;

  case PACKET_RULESET_TERRAIN_CONTROL:
    freelog(LOG_VERBOSE, "handle_ruleset_terrain_control");
    handle_ruleset_terrain_control(packet);
    return TRUE;

  case PACKET_RULESET_NATION:
    freelog(LOG_VERBOSE, "handle_ruleset_nation");
    handle_ruleset_nation(packet);
    return TRUE;

  case PACKET_RULESET_CITY:
    freelog(LOG_VERBOSE, "handle_ruleset_city");
    handle_ruleset_city(packet);
    return TRUE;

  case PACKET_RULESET_BUILDING:
    freelog(LOG_VERBOSE, "handle_ruleset_building");
    handle_ruleset_building(packet);
    return TRUE;

  case PACKET_RULESET_TERRAIN:
    freelog(LOG_VERBOSE, "handle_ruleset_terrain");
    handle_ruleset_terrain(packet);
    return TRUE;

  case PACKET_RULESET_CONTROL:
    freelog(LOG_VERBOSE, "handle_ruleset_control");
    handle_ruleset_control(packet);
    return TRUE;

  case PACKET_SINGLE_WANT_HACK_REPLY:
    freelog(LOG_VERBOSE, "handle_single_want_hack_reply");
    handle_single_want_hack_reply(
      ((struct packet_single_want_hack_reply *)packet)->you_have_hack);
    return TRUE;

  case PACKET_GAME_LOAD:
    freelog(LOG_VERBOSE, "handle_game_load");
    handle_game_load(packet);
    return TRUE;

  case PACKET_OPTIONS_SETTABLE_CONTROL:
    freelog(LOG_VERBOSE, "handle_options_settable_control");
    handle_options_settable_control(packet);
    return TRUE;

  case PACKET_OPTIONS_SETTABLE:
    freelog(LOG_VERBOSE, "handle_options_settable");
    handle_options_settable(packet);
    return TRUE;

  case PACKET_RULESET_CACHE_GROUP:
    freelog(LOG_VERBOSE, "handle_ruleset_cache_group");
    handle_ruleset_cache_group(packet);
    return TRUE;

  case PACKET_RULESET_CACHE_EFFECT:
    freelog(LOG_VERBOSE, "handle_ruleset_cache_effect");
    handle_ruleset_cache_effect(packet);
    return TRUE;

  case PACKET_TRADEROUTE_INFO:
    freelog(LOG_VERBOSE, "handle_traderoute_info");
    handle_traderoute_info(packet);
    return TRUE;

  case PACKET_EXTGAME_INFO:
    freelog(LOG_VERBOSE, "handle_extgame_info");
    handle_extgame_info(packet);
    return TRUE;

  default:
    return FALSE;
  }
}
