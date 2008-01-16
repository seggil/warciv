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


/* Calibrated so that if a player with RD 50 does
 * not play for 3 years, his RD will become 350.
 * A 'rating period' is assumed to be a typical
 * game length, i.e. 3 hours. */
#define RATING_CONSTANT_SECONDS_PER_RATING_PERIOD 10800
#define RATING_CONSTANT_C 3.69993869
#define RATING_CONSTANT_Q 0.00575646273 /* ln(10)/400 */

/* The RD given to new players. */
#define RATING_CONSTANT_MAXIMUM_RD 350

/* To ensure that ratings do not update too slowly
 * if you play lots of games, RDs will not fall below this
 * value. */
#define RATING_CONSTANT_MINIMUM_RD 30

/* How much should the difference between a team member's
 * rating and the average player rating affect the rating
 * of the team as a whole?
 * (See the relevant section in update_ratings for
 * how it is used). */
#define RATING_CONSTANT_EXTRA_TEAMMATE_INFLUENCE 0.3

/* The rating given to new players. */
#define RATING_CONSTANT_AVERAGE_PLAYER_RATING 1500

/* The rating at which a player becomes more of a
 * liability to his teammates rather than a boon. */
#define RATING_CONSTANT_BAD_PLAYER_RATING 1000

/* Used to determine the 'rating' of the 'opponent' for
 * solo games. Calibrated so that an average player (1500)
 * can win a solo game at turn 100 with score 300. */
#define RATING_CONSTANT_SOLO_RATING_COEFFICIENT 505

/* Players with RDs higher than this are said to have
 * a "provisional" rating, i.e. their rating should not
 * taken as an accurate estimate of their real skill. */
#define RATING_CONSTANT_RELIABLE_RD 100.0

/* Scores differing by less than this are considered equal. */
#define MINIMUM_SCORE_DIFFERENCE 1.0


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
