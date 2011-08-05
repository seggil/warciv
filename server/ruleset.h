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
#ifndef WC_SERVER_RULESET_H
#define WC_SERVER_RULESET_H

#define MAX_NUM_RULESETS 32

struct conn_list;

void load_rulesets(void);
void send_rulesets(struct conn_list *dest);

bool is_valid_ruleset(const char *path, char *verror, size_t verror_size,
                      bool check_capstr);
char **get_rulesets_list(void);
char *get_ruleset_description(const char *ruleset);

#endif  /* WC_SERVER_RULESET_H */
