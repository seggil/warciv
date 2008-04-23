/**********************************************************************
 Freeciv - Copyright (C) 2005 - M.C. Kaufman
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__DATABASE_H
#define FC__DATABASE_H

#include "shared.h"

struct connection;

bool is_guest_name(const char *name);
void get_unique_guest_name(char *name);

bool authenticate_user(struct connection *pconn, char *username);
void process_authentication_status(struct connection *pconn);
bool handle_authentication_reply(struct connection *pc, char *password);

struct fcdb_params {
  char host[256];
  char user[256];
  char password[256];
  char dbname[256];
};
extern struct fcdb_params fcdb;

struct game_type_stats {
  char type[32];
  int count;
  int wins;
  int loses;
  int draws;
  double rating;
  double rating_deviation;
};

struct fcdb_user_stats {
  int id;
  char username[MAX_LEN_NAME];
  char email[128]; /* Currently never set in database. :( */
  time_t createtime;
  time_t accesstime;
  char address[MAX_LEN_ADDR];
  char createaddress[MAX_LEN_ADDR];
  int logincount;

  struct game_type_stats *gt_stats;
  int num_game_types;
};

struct fcdb_team_in_game_info {
  char name[MAX_LEN_NAME];
  char result[32];
  float rank;
  float score;
};

struct fcdb_player_in_game_info {
  char name[MAX_LEN_NAME];
  char user[MAX_LEN_NAME];
  char nation[32];
  char team_name[MAX_LEN_NAME];
  char result[32];
  float rank;
  float score;
  double old_rating;
  double old_rd;
  double new_rating;
  double new_rd;
};

struct fcdb_game_info {
  int id;
  char host[64];
  int port;
  char type[32];
  char outcome[64];
  bool completed;
  time_t created;
  time_t duration;
  int num_turns;
  int num_players;
  struct fcdb_player_in_game_info *players;
  int num_teams;
  struct fcdb_team_in_game_info *teams;
};

struct fcdb_topten_info_entry {
  int id;
  char user[MAX_LEN_NAME];
  double rating, rd;
  int wins, loses, draws;
};

struct fcdb_topten_info {
  int count;
  struct fcdb_topten_info_entry *entries;
};

struct fcdb_gamelist_entry {
  int id;
  char type[32];
  int players;
  time_t created;
  char outcome[64];
};

struct fcdb_gamelist {
  int count;
  int id;
  struct fcdb_gamelist_entry *entries;
};

struct fcdb_aliaslist_entry {
  int id;
  char name[MAX_LEN_NAME];
};

struct fcdb_aliaslist {
  int count;
  int id;
  struct fcdb_aliaslist_entry *entries;
};

/* Forward declaration. */
struct string_list;

bool fcdb_record_game_start(void);
bool fcdb_end_of_turn_update(void);
bool fcdb_record_game_end(void);
bool fcdb_load_player_ratings(int game_type, bool check_turns_played);
struct fcdb_user_stats *fcdb_user_stats_new(const char *username,
                                            struct string_list *matchs);
void fcdb_user_stats_free(struct fcdb_user_stats *fus);
int fcdb_user_exists(const char *username);
bool fcdb_get_user_rating(const char *username,
                          int game_type,
                          double *prating,
                          double *prating_deviation);
bool fcdb_get_recent_games(const char *username,
                           int *recent_games,
                           int *count);
struct fcdb_game_info *fcdb_game_info_new(int id);
void fcdb_game_info_free(struct fcdb_game_info *fgi);
struct fcdb_topten_info *fcdb_topten_info_new(int type);
void fcdb_topten_info_free(struct fcdb_topten_info *ftti);
struct fcdb_gamelist *fcdb_gamelist_new(int type, const char *user,
                                        int first, int last);
void fcdb_gamelist_free(struct fcdb_gamelist *fgl);
struct fcdb_aliaslist *fcdb_aliaslist_new(const char *user);
void fcdb_aliaslist_free(struct fcdb_aliaslist *fal);

#endif /* FC__DATABASE_H */
