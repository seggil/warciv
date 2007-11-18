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
#ifndef FC__MENU_G_H
#define FC__MENU_G_H

void update_menus(void);
void init_menus(void);

void update_delayed_goto_menu(int i);
void update_airlift_menu(int i);
void update_auto_caravan_menu(void);
void update_multi_selection_menu(int i);
void update_miscellaneous_menu(void);
void start_turn_menus_udpate(void);

#endif  /* FC__MENU_G_H */
