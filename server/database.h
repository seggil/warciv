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
#ifndef WC_SERVER_DATABASE_H
#define WC_SERVER_DATABASE_H

#include "shared.h"

void database_init(void);
void database_free(void);
void database_reload(void);

bool is_guest_name(const char *name);
void get_unique_guest_name(char *name);

struct connection;

bool authenticate_user(struct connection *pconn, char *username);
void process_authentication_status(struct connection *pconn);
bool handle_authentication_reply(struct connection *pc, char *password);
bool wcdb_check_salted_passwords(void);

struct wcdb_params {
  char host[256];
  char user[256];
  char password[256];
  char dbname[256];
};
extern struct wcdb_params wcdb;

struct game_type_stats {
  char type[32];
  int count;
  int wins;
  int loses;
  int draws;
  double rating;
  double rating_deviation;
};

struct wcdb_user_stats {
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

struct wcdb_team_in_game_info {
  char name[MAX_LEN_NAME];
  char result[32];
  float rank;
  float score;
};

struct wcdb_player_in_game_info {
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

struct wcdb_game_info {
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
  struct wcdb_player_in_game_info *players;
  int num_teams;
  struct wcdb_team_in_game_info *teams;
};

struct wcdb_topten_info_entry {
  int id;
  char user[MAX_LEN_NAME];
  double rating, rd;
  int wins, loses, draws;
};

struct wcdb_topten_info {
  int count;
  struct wcdb_topten_info_entry *entries;
};

struct wcdb_gamelist_entry {
  int id;
  char type[32];
  int players;
  time_t created;
  char outcome[64];
};

struct wcdb_gamelist {
  int count;
  int id;
  struct wcdb_gamelist_entry *entries;
};

struct wcdb_aliaslist_entry {
  int id;
  char name[MAX_LEN_NAME];
};

struct wcdb_aliaslist {
  int count;
  int id;
  struct wcdb_aliaslist_entry *entries;
};

/* Forward declaration. */
struct string_list;

bool wcdb_record_game_start(void);
bool wcdb_end_of_turn_update(void);
bool wcdb_record_game_end(void);
bool wcdb_load_player_ratings(int game_type, bool check_turns_played);
struct wcdb_user_stats *wcdb_user_stats_new(const char *username,
                                            struct string_list *matchs);
void wcdb_user_stats_free(struct wcdb_user_stats *fus);
int wcdb_user_exists(const char *username);
bool wcdb_get_user_rating(const char *username,
                          int game_type,
                          double *prating,
                          double *prating_deviation);
bool wcdb_get_recent_games(const char *username,
                           int *recent_games,
                           int *count);
struct wcdb_game_info *wcdb_game_info_new(int id);
void wcdb_game_info_free(struct wcdb_game_info *fgi);
struct wcdb_topten_info *wcdb_topten_info_new(int type);
void wcdb_topten_info_free(struct wcdb_topten_info *ftti);
struct wcdb_gamelist *wcdb_gamelist_new(int type, const char *user,
                                        int first, int last);
void wcdb_gamelist_free(struct wcdb_gamelist *fgl);
struct wcdb_aliaslist *wcdb_aliaslist_new(const char *user);
void wcdb_aliaslist_free(struct wcdb_aliaslist *fal);

#endif  /* WC_SERVER_DATABASE_H */
