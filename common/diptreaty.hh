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
#ifndef WC_COMMON_DIPTREATY_H
#define WC_COMMON_DIPTREATY_H

#include "shared.h"             /* bool type */

enum clause_type { CLAUSE_ADVANCE, CLAUSE_GOLD, CLAUSE_MAP,
                   CLAUSE_SEAMAP, CLAUSE_CITY,
                   CLAUSE_CEASEFIRE, CLAUSE_PEACE, CLAUSE_ALLIANCE,
                   CLAUSE_VISION, CLAUSE_UNUSED, CLAUSE_EMBASSY,
                   CLAUSE_LAST };

#define is_pact_clause(x)                                                   \
  ((x == CLAUSE_CEASEFIRE) || (x == CLAUSE_PEACE) || (x == CLAUSE_ALLIANCE))

/* For when we need to iterate over treaties */
struct Clause {
  enum clause_type type;
  player_t *from;
  int value;
};

#define SPECLIST_TAG clause
#define SPECLIST_TYPE struct Clause
#include "speclist.h"

#define clause_list_iterate(clauselist, pclause) \
    TYPED_LIST_ITERATE(struct Clause, clauselist, pclause)
#define clause_list_iterate_end  LIST_ITERATE_END

struct Treaty {
  player_t *plr0, *plr1;
  bool accept0, accept1;
  struct clause_list *clauses;
};

bool diplomacy_possible(player_t *pplayer, player_t *aplayer);
bool could_meet_with_player(player_t *pplayer, player_t *aplayer);
bool could_intel_with_player(player_t *pplayer, player_t *aplayer);

struct Treaty *treaty_new(player_t *plr0, player_t *plr1);
bool add_clause(struct Treaty *ptreaty, player_t *pfrom,
                enum clause_type type, int val);
bool remove_clause(struct Treaty *ptreaty, player_t *pfrom,
                   enum clause_type type, int val);
void treaty_free(struct Treaty *ptreaty);

#endif  /* WC_COMMON_DIPTREATY_H */
