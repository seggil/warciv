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


/* In the terminology of the Glicko rating system,
 * we set a rating period to be 1 day and calibrate
 * the constant 'c' so that a player's rating becomes
 * completely unreliable (RD=350) if that player does
 * not play for a year.
 *
 * The formula to calculate a new c value is given by
 *
 * c = sqrt((350^2 - 50^2) / t_u)
 *   = 346.410162 / sqrt(t_u)
 *
 * where t_u is the number of ratings periods in
 * the time that it takes for a player's rating to
 * become completely unreliable (in our case t_u =
 * 365, i.e. the number of days in a year). */
#define RATING_CONSTANT_SECONDS_PER_RATING_PERIOD 86400
#define RATING_CONSTANT_C 18.1319365819151

/* ln(10)/400 */
#define RATING_CONSTANT_Q 0.00575646273

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
 * be taken as an "accurate" estimate of their real
 * skill. This is currently only used by the /autoteam
 * command to determine whether to use individual player
 * ratings for ordering. */
#define RATING_CONSTANT_RELIABLE_RD 300.0

/* A user must play for at least this many turns before
 * being eligible for a rating update. */
#define RATING_CONSTANT_PLAYER_MINIMUM_TURN_COUNT 30

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
