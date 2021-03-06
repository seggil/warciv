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
#ifndef WC_CLIENT_CLIMISC_H
#define WC_CLIENT_CLIMISC_H

#include "shared.hh"          /* MAX_LEN_NAME */

#include "wc_types.hh"
#include "events.hh"

struct Clause;


enum tag_link_types {
  LINK_LOCATION = 1,
  LINK_CITY,
  LINK_CITY_ID,
  LINK_CITY_ID_AND_NAME,
  LINK_UNIT
};

void client_remove_player(int plrno);
void client_remove_city(city_t *pcity);
void client_remove_unit(unit_t *punit);

const char *get_embassy_status(player_t *me, player_t *them);
const char *get_vision_status(player_t *me, player_t *them);
void client_diplomacy_clause_string(char *buf, int bufsiz,
                                    struct Clause *pclause);
char *get_tile_info(tile_t *ptile);

int client_research_sprite(void);
int client_warming_sprite(void);
int client_cooling_sprite(void);

void center_on_something(void);

/*
 * A city compound id (city_cid) can hold all objects a city can build:
 * improvements (with wonders) and units. This is achieved by
 * seperation the value set: a city_cid < B_LAST denotes a improvement
 * (including wonders). A city_cid >= B_LAST denotes a unit with the
 * unit_type_id of (city_cid - B_LAST).
 */

typedef unsigned int city_cid;    /* city compound id */
city_cid city_cid_encode(bool is_unit, int id);
city_cid city_cid_encode_from_city(city_t *pcity);
void     city_cid_decode(city_cid cid, bool *is_unit, int *id);
bool     city_cid_is_unit(city_cid cid);
int      city_cid_id(city_cid cid);

void     client_change_all(city_cid x, city_cid y);

/*
 * A worklist_id can hold all objects which can be part of a
 * city worklist: improvements (with wonders), units and global
 * worklists. This is achieved by seperation the value set:
 *  - (worklist_id < B_LAST) denotes a improvement (including wonders)
 *  - (B_LAST <= worklist_id < B_LAST + U_LAST) denotes a unit with the
 *    unit_type_id of (worklist_id - B_LAST)
 *  - (B_LAST + U_LAST<= worklist_id) denotes a global worklist with
 *    the id of (worklist_id - (B_LAST + U_LAST))
 */

typedef int worklist_id;
#define WORKLIST_END (-1)

worklist_id worklist_id_encode(bool is_unit, bool is_worklist, int id);
bool worklist_id_is_unit(worklist_id wid);
bool worklist_id_is_worklist(worklist_id wid);
int  worklist_id_id(worklist_id wid);

bool city_can_build_impr_or_unit(city_t *pcity, city_cid cid);
bool city_unit_supported(city_t *pcity, city_cid cid);
bool city_unit_present(city_t *pcity, city_cid cid);
bool city_building_present(city_t *pcity, city_cid cid);
bool city_can_sell_impr(city_t *pcity, city_cid cid);

struct item {
  city_cid cid_;
  char descr[MAX_LEN_NAME + 40];

  /* Privately used for sorting */
  int section;
};

void name_and_sort_items(city_cid *pcity_cids, int num_cids,
                         struct item *items,
                         bool show_cost, city_t *pcity);
int collect_city_cids1(city_cid * dest_city_cids,
                       city_t **selected_cities,
                       int num_selected_cities, bool append_units,
                       bool append_wonders, bool change_prod,
                       bool (*test_func) (city_t *, city_cid));
int collect_city_cids2(city_cid *dest_city_cids);
int collect_city_cids3(city_cid *dest_city_cids);
int collect_city_cids4(city_cid *dest_city_cids,
                       city_t *pcity,
                       bool advanced_tech);
int collect_city_cids5(city_cid *dest_city_cids, city_t *pcity);

/* the number of units in city */
int num_present_units_in_city(city_t* pcity);
int num_supported_units_in_city(city_t* pcity);

void handle_event(char *message, tile_t *ptile,
                  enum event_type event, int conn_id);
void create_event(tile_t *ptile, enum event_type event,
                  const char *format, ...)
     wc__attribute((__format__ (__printf__, 3, 4)));
void write_chatline_content(const char *txt);

void reports_freeze(void);
void reports_freeze_till(int request_id);
void reports_thaw(void);
void reports_force_thaw(void);

city_t *get_nearest_city(unit_t *punit, int *sq_dist);

void cityrep_buy(city_t *pcity);
void common_taxrates_callback(int i);

int buy_production_in_selected_cities(void);
void set_rally_point_for_selected_cities(tile_t *ptile);

void city_clear_worklist(city_t *pcity);
void clear_worklists_in_selected_cities(void);
void city_worklist_check(city_t *pcity, struct worklist *pwl);

void city_autonaming_init(void);
void city_autonaming_free(void);
void city_autonaming_add_used_name(const char *city_name);
void city_autonaming_remove_used_name(const char *city_name);
void normalize_names_in_selected_cities(void);
void toggle_traderoute_drawing_in_selected_cities(void);

void link_marks_init(void);
void link_marks_free(void);
void clear_all_link_marks(void);
void decrease_link_mark_turn_counters(void);
void draw_all_link_marks(void);
void add_link_mark(enum tag_link_types type, int id);
void restore_link_mark(enum tag_link_types type, int id);

void set_default_user_tech_goal(void);
void force_tech_goal(Tech_Type_id goal);

void execute_air_patrol_orders(void);
void do_unit_air_patrol(unit_t *punit, tile_t *ptile);

enum client_vote_type {
  CLIENT_VOTE_NONE = 0,
  CLIENT_VOTE_YES,
  CLIENT_VOTE_NO,
  CLIENT_VOTE_ABSTAIN
};

struct voteinfo {
  /* Set by the server via packets. */
  int vote_no;
  char user[MAX_LEN_NAME];
  char desc[512];
  int percent_required;
  int flags;
  bool is_poll;
  int yes;
  int no;
  int abstain;
  int num_voters;
  bool resolved;
  bool passed;

  /* Set/used by the client. */
  enum client_vote_type client_vote;
  time_t remove_time;
};

void voteinfo_queue_init(void);
void voteinfo_queue_free(void);
void voteinfo_queue_remove(int vote_no);
void voteinfo_queue_delayed_remove(int vote_no);
void voteinfo_queue_check_removed(void);
void voteinfo_queue_add(int vote_no,
                        const char *user,
                        const char *desc,
                        int percent_required,
                        int flags,
                        bool is_poll);
struct voteinfo *voteinfo_queue_find(int vote_no);
void voteinfo_do_vote(int vote_no, enum client_vote_type vote);
struct voteinfo *voteinfo_queue_get_current(int *pindex);
void voteinfo_queue_next(void);

/* Define struct voteinfo_list type. */
#define SPECLIST_TAG voteinfo
#define SPECLIST_TYPE struct voteinfo
#include "speclist.hh"
#define voteinfo_list_iterate(alist, pitem)\
  TYPED_LIST_ITERATE(struct voteinfo, alist, pitem)
#define voteinfo_list_iterate_end  LIST_ITERATE_END

extern struct voteinfo_list *voteinfo_queue;

#endif /* WC_CLIENT_CLIMISC_H */
