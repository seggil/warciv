
 /****************************************************************************
 *                       THIS FILE WAS GENERATED                             *
 * Script: common/generate_packets.py                                        *
 * Input:  common/packets.def                                                *
 *                       DO NOT CHANGE THIS FILE                             *
 ****************************************************************************/


#ifndef WC_SERVER_HAND_GEN_H
#define WC_SERVER_HAND_GEN_H

#include "shared.h"

#include "wc_types.h"
#include "packets.h"

struct connection;

bool server_handle_packet(enum packet_type type, void *packet,
                          player_t *pplayer, struct connection *pconn);

void handle_nation_select_req(player_t *pplayer, Nation_Type_id nation_no, bool is_male, char *name, int city_style);
void handle_chat_msg_req(struct connection *pconn, char *message);
void handle_city_sell(player_t *pplayer, int city_id, int build_id);
void handle_city_buy(player_t *pplayer, int city_id);
void handle_city_change(player_t *pplayer, int city_id, int build_id, bool is_build_id_unit_id);
void handle_city_worklist(player_t *pplayer, int city_id, struct worklist *worklist);
void handle_city_make_specialist(player_t *pplayer, int city_id, int worker_x, int worker_y);
void handle_city_make_worker(player_t *pplayer, int city_id, int worker_x, int worker_y);
void handle_city_change_specialist(player_t *pplayer, int city_id, Specialist_type_id from, Specialist_type_id to);
void handle_city_rename(player_t *pplayer, int city_id, char *name);
void handle_city_options_req(player_t *pplayer, int city_id, int value);
void handle_city_refresh(player_t *pplayer, int city_id);
void handle_city_incite_inq(struct connection *pconn, int city_id);
void handle_city_name_suggestion_req(player_t *pplayer, int unit_id);
void handle_player_turn_done(player_t *pplayer);
void handle_player_rates(player_t *pplayer, int tax, int luxury, int science);
void handle_player_change_government(player_t *pplayer, int government);
void handle_player_research(player_t *pplayer, int tech);
void handle_player_tech_goal(player_t *pplayer, int tech);
void handle_player_attribute_block(player_t *pplayer);
struct packet_player_attribute_chunk;
void handle_player_attribute_chunk(player_t *pplayer, struct packet_player_attribute_chunk *packet);
void handle_unit_move(player_t *pplayer, int unit_id, int x, int y);
void handle_unit_build_city(player_t *pplayer, int unit_id, char *name);
void handle_unit_disband(player_t *pplayer, int unit_id);
void handle_unit_change_homecity(player_t *pplayer, int unit_id, int city_id);
void handle_unit_establish_trade(player_t *pplayer, int unit_id);
void handle_unit_help_build_wonder(player_t *pplayer, int unit_id);
void handle_unit_goto(player_t *pplayer, int unit_id, int x, int y);
struct packet_unit_orders;
void handle_unit_orders(player_t *pplayer, struct packet_unit_orders *packet);
void handle_unit_auto(player_t *pplayer, int unit_id);
void handle_unit_load(player_t *pplayer, int cargo_id, int transporter_id);
void handle_unit_unload(player_t *pplayer, int cargo_id, int transporter_id);
void handle_unit_upgrade(player_t *pplayer, int unit_id);
void handle_unit_nuke(player_t *pplayer, int unit_id);
void handle_unit_paradrop_to(player_t *pplayer, int unit_id, int x, int y);
void handle_unit_airlift(player_t *pplayer, int unit_id, int city_id);
void handle_unit_bribe_inq(struct connection *pconn, int unit_id);
void handle_unit_type_upgrade(player_t *pplayer, Unit_Type_id type);
void handle_unit_diplomat_action(player_t *pplayer, int diplomat_id, enum diplomat_actions action_type, int target_id, int value);
void handle_unit_change_activity(player_t *pplayer, int unit_id, enum unit_activity activity, enum tile_special_type activity_target);
void handle_diplomacy_init_meeting_req(player_t *pplayer, int counterpart);
void handle_diplomacy_cancel_meeting_req(player_t *pplayer, int counterpart);
void handle_diplomacy_create_clause_req(player_t *pplayer, int counterpart, int giver, enum clause_type type, int value);
void handle_diplomacy_remove_clause_req(player_t *pplayer, int counterpart, int giver, enum clause_type type, int value);
void handle_diplomacy_accept_treaty_req(player_t *pplayer, int counterpart);
void handle_diplomacy_cancel_pact(player_t *pplayer, int other_player_id, enum clause_type clause);
void handle_report_req(struct connection *pconn, enum report_type type);
void handle_conn_pong(struct connection *pconn);
void handle_spaceship_launch(player_t *pplayer);
void handle_spaceship_place(player_t *pplayer, enum spaceship_place_type type, int num);
void handle_vote_submit(struct connection *pconn, int vote_no, int value);
void handle_trade_route_plan(player_t *pplayer, int city1, int city2);
void handle_trade_route_remove(player_t *pplayer, int city1, int city2);
void handle_unit_trade_route(player_t *pplayer, int unit_id, int city1, int city2);
void handle_city_set_rally_point(player_t *pplayer, int id, int x, int y);
void handle_city_clear_rally_point(player_t *pplayer, int id);
void handle_unit_air_patrol(player_t *pplayer, int id, int x, int y);
void handle_unit_air_patrol_stop(player_t *pplayer, int id);
struct packet_city_manager_param;
void handle_city_manager_param(player_t *pplayer, struct packet_city_manager_param *packet);
void handle_city_no_manager_param(player_t *pplayer, int id);
void handle_player_info_req(struct connection *pconn, int id);

#endif /* WC_SERVER_HAND_GEN_H */
