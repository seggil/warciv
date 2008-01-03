/********************************************************************** 
 Freeciv - Copyright (C) 2003 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__SCORE_H
#define FC__SCORE_H

#include "fc_types.h"

/* Used in ranking and rating calculation. */
struct grouping {
  double score;
  struct player *players[MAX_NUM_PLAYERS];
  int num_players;
  int num_alive;
  int result;
  double rank;
  double rank_offset;

  double rating;
  double rating_deviation; /* i.e. standard deviation for the rating */
  double new_rating;
  double new_rating_deviation;
};

void calc_civ_score(struct player *pplayer);

int get_civ_score(const struct player *pplayer);
int total_player_citizens(const struct player *pplayer);

void save_ppm(void);

void score_evaluate_players(void);
void score_calculate_team_scores(void);
void score_assign_new_player_rating(struct player *pplayer,
                                    int game_type);
void score_assign_ai_rating(struct player *pplayer,
                            int game_type);
void score_update_grouping_results(void);
void score_assign_groupings(void);
void score_propagate_grouping_results(void);
double score_get_solo_opponent_rating_deviation(void);
double score_calculate_solo_opponent_rating(const struct grouping *g);
void score_get_ai_rating(int skill_level, int game_type,
                         double *prating, double *prd);
void score_calculate_grouping_ratings(void);
void score_propagate_grouping_ratings(void);

const struct grouping *score_get_groupings(int *num_groupings);

int player_get_rated_username(const struct player *pplayer,
                              char *outbuf, int maxlen);

#endif /* FC__SCORE_H */
