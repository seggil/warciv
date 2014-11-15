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
#ifndef WC_SERVER_PLRHAND_H
#define WC_SERVER_PLRHAND_H

#include <stdarg.h>

#include "shared.hh"             /* wc__attribute */

#include "events.hh"
#include "wc_types.hh"
#include "packets.hh"

#include "hand_gen.hh"

struct section_file;

enum plr_info_level { INFO_MINIMUM, INFO_MEETING, INFO_EMBASSY, INFO_FULL };

void server_player_init(player_t *pplayer, bool initmap);
void server_remove_player(player_t *pplayer);
void kill_player(player_t *pplayer);
void kill_dying_players(void);
void update_revolution(player_t *pplayer);
void do_tech_parasite_effect(player_t *pplayer);
void check_player_government_rates(player_t *pplayer);
void make_contact(player_t *pplayer1, player_t *pplayer2,
                  tile_t *ptile);
void maybe_make_contact(tile_t *ptile, player_t *pplayer);

void send_player_info(player_t *src, player_t *dest);
void send_player_info_c(player_t *src, struct connection_list *dest);

void notify_conn_ex(struct connection_list *dest, tile_t *ptile,
                    enum event_type event, const char *format, ...)
                    wc__attribute((__format__ (__printf__, 4, 5)));
void vnotify_conn_ex(struct connection_list *dest, tile_t *ptile,
                     enum event_type event, const char *format,
                     va_list vargs);
void notify_conn(struct connection_list *dest, const char *format, ...)
                 wc__attribute((__format__ (__printf__, 2, 3)));
void notify_player_ex(const player_t *pplayer, tile_t *ptile,
                      enum event_type event, const char *format, ...)
                      wc__attribute((__format__ (__printf__, 4, 5)));
void notify_player(const player_t *pplayer, const char *format, ...)
                   wc__attribute((__format__ (__printf__, 2, 3)));
void notify_embassies(player_t *pplayer, player_t *exclude,
                      const char *format, ...)
                      wc__attribute((__format__ (__printf__, 3, 4)));
void notify_team(const struct team *pteam, const char *format, ...)
                 wc__attribute((__format__ (__printf__, 2, 3)));
void notify_enabled(bool yes);

struct connection_list *player_reply_dest(player_t *pplayer);

void found_new_tech(player_t *plr, int tech_found, bool was_discovery,
                    bool saving_bulbs, int next_tech);
void found_new_future_tech(player_t *pplayer);
void update_tech(player_t *plr, int bulbs);
void init_tech(player_t *plr);
void choose_random_tech(player_t *plr);
Tech_Type_id choose_goal_tech(player_t *plr);
void choose_tech(player_t *plr, int tech);
void choose_tech_goal(player_t *plr, int tech);
void get_a_tech(player_t *pplayer, player_t *target);
void give_initial_techs(player_t* plr);
Tech_Type_id give_random_initial_tech(player_t* pplayer);
Tech_Type_id give_random_free_tech(player_t* pplayer);

void send_player_turn_notifications(struct connection_list *dest);

void do_dipl_cost(player_t *pplayer, Tech_Type_id new_tech);
void do_free_cost(player_t *pplayer);
void do_conquer_cost(player_t *pplayer);

void shuffle_players(void);
void set_shuffled_players(int *shuffled_players);
player_t *shuffled_player(int i);

#define shuffled_players_iterate(pplayer)                                   \
{                                                                           \
  player_t *pplayer;                                                        \
  int i;                                                                    \
  for (i = 0; i < game.info.nplayers; i++) {                                \
    pplayer = shuffled_player(i);                                           \
    {

#define shuffled_players_iterate_end                                        \
    }                                                                       \
  }                                                                         \
}

bool civil_war_triggered(player_t *pplayer);
void civil_war(player_t *pplayer);

void update_players_after_alliance_breakup(player_t *pplayer,
                                           player_t *pplayer2);
#endif  /* WC_SERVER_PLRHAND_H */
