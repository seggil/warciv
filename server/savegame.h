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
#ifndef WC_SERVER_SAVEGAME_H
#define WC_SERVER_SAVEGAME_H

#include "registry.h"

#define REQUIERED_GAME_VERSION 10900

void game_load(struct section_file *file);
bool game_loadmap(struct section_file *file);
void game_save(struct section_file *file);

#endif  /* WC_SERVER_SAVEGAME_H */
