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
#ifndef WC_SERVER_REPORT_H
#define WC_SERVER_PLRHAND_H

#include "shared.hh"             /* bool type */

void page_conn(struct connection_list *dest, const char *caption, const char *headline,
               const char *lines);

void make_history_report(void);
void report_wonders_of_the_world(struct connection_list *dest);
void report_top_five_cities(struct connection_list *dest);
bool is_valid_demography(const char *demography, const char **error_message);
void report_demographics(connection_t *pconn);
void report_progress_scores(void);
void report_final_scores(struct connection_list *dest);
void report_game_rankings(struct connection_list *dest);

/* See also report_server_options() in stdinhand.h */

#endif  /* WC_SERVER_PLRHAND_H */
