
 /****************************************************************************
 *                       THIS FILE WAS GENERATED                             *
 * Script: common/generate_packets.py                                        *
 * Input:  common/packets.def                                                *
 *                       DO NOT CHANGE THIS FILE                             *
 ****************************************************************************/


#ifndef FC__PACKHAND_GEN_H
#define FC__PACKHAND_GEN_H

#include "packets.h"
#include "shared.h"

bool client_handle_packet(enum packet_type type, void *packet);

void handle_processing_started(void);
void handle_processing_finished(void);
void handle_freeze_hint(void);
void handle_thaw_hint(void);
void handle_server_join_reply(bool you_can_join, char *message, char *capability, char *challenge_file, int conn_id);
void handle_authentication_req(enum authentication_type type, char *message);
void handle_server_shutdown(void);
void handle_nation_unavailable(Nation_Type_id nation);
void handle_select_races(void);
void handle_nation_select_ok(void);
void handle_game_state(int value);
struct packet_endgame_report;
void handle_endgame_report(struct packet_endgame_report *packet);
struct packet_tile_info;
void handle_tile_info(struct packet_tile_info *packet);
struct packet_game_info;
void handle_game_info(struct packet_game_info *packet);
void handle_map_info(int xsize, int ysize, int topology_id);
void handle_nuke_tile_info(int x, int y);
void handle_chat_msg(char *message, int x, int y, enum event_type event, int conn_id);
void handle_city_remove(int city_id);
struct packet_city_info;
void handle_city_info(struct packet_city_info *packet);
struct packet_city_short_info;
void handle_city_short_info(struct packet_city_short_info *packet);
void handle_city_incite_info(int city_id, int cost);
void handle_city_name_suggestion_info(int unit_id, char *name);
void handle_city_sabotage_list(int diplomat_id, int city_id, char *improvements);
void handle_player_remove(int player_id);
struct packet_player_info;
void handle_player_info(struct packet_player_info *packet);
struct packet_player_attribute_chunk;
void handle_player_attribute_chunk(struct packet_player_attribute_chunk *packet);
void handle_unit_remove(int unit_id);
struct packet_unit_info;
void handle_unit_info(struct packet_unit_info *packet);
struct packet_unit_short_info;
void handle_unit_short_info(struct packet_unit_short_info *packet);
void handle_unit_combat_info(int attacker_unit_id, int defender_unit_id, int attacker_hp, int defender_hp, bool make_winner_veteran);
void handle_unit_bribe_info(int unit_id, int cost);
void handle_unit_diplomat_popup_dialog(int diplomat_id, int target_id);
void handle_diplomacy_init_meeting(int counterpart, int initiated_from);
void handle_diplomacy_cancel_meeting(int counterpart, int initiated_from);
void handle_diplomacy_create_clause(int counterpart, int giver, enum clause_type type, int value);
void handle_diplomacy_remove_clause(int counterpart, int giver, enum clause_type type, int value);
void handle_diplomacy_accept_treaty(int counterpart, bool I_accepted, bool other_accepted);
void handle_page_msg(char *message, enum event_type event);
struct packet_conn_info;
void handle_conn_info(struct packet_conn_info *packet);
struct packet_conn_ping_info;
void handle_conn_ping_info(struct packet_conn_ping_info *packet);
void handle_conn_ping(void);
void handle_before_new_year(void);
void handle_start_turn(void);
void handle_new_year(int year, int turn);
struct packet_spaceship_info;
void handle_spaceship_info(struct packet_spaceship_info *packet);
struct packet_ruleset_unit;
void handle_ruleset_unit(struct packet_ruleset_unit *packet);
struct packet_ruleset_game;
void handle_ruleset_game(struct packet_ruleset_game *packet);
struct packet_ruleset_government_ruler_title;
void handle_ruleset_government_ruler_title(struct packet_ruleset_government_ruler_title *packet);
struct packet_ruleset_tech;
void handle_ruleset_tech(struct packet_ruleset_tech *packet);
struct packet_ruleset_government;
void handle_ruleset_government(struct packet_ruleset_government *packet);
struct packet_ruleset_terrain_control;
void handle_ruleset_terrain_control(struct packet_ruleset_terrain_control *packet);
struct packet_ruleset_nation;
void handle_ruleset_nation(struct packet_ruleset_nation *packet);
struct packet_ruleset_city;
void handle_ruleset_city(struct packet_ruleset_city *packet);
struct packet_ruleset_building;
void handle_ruleset_building(struct packet_ruleset_building *packet);
struct packet_ruleset_terrain;
void handle_ruleset_terrain(struct packet_ruleset_terrain *packet);
struct packet_ruleset_control;
void handle_ruleset_control(struct packet_ruleset_control *packet);
void handle_single_want_hack_reply(bool you_have_hack);
struct packet_game_load;
void handle_game_load(struct packet_game_load *packet);
struct packet_options_settable_control;
void handle_options_settable_control(struct packet_options_settable_control *packet);
struct packet_options_settable;
void handle_options_settable(struct packet_options_settable *packet);
struct packet_ruleset_cache_group;
void handle_ruleset_cache_group(struct packet_ruleset_cache_group *packet);
struct packet_ruleset_cache_effect;
void handle_ruleset_cache_effect(struct packet_ruleset_cache_effect *packet);

#endif /* FC__PACKHAND_GEN_H */
