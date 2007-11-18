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
#ifndef FC__MAPCTRL_H
#define FC__MAPCTRL_H

#include <X11/Intrinsic.h>

#include "mapctrl_g.h"


void mapctrl_key_city_workers(XEvent *event);

void mapctrl_btn_mapcanvas(XEvent *event);
void mapctrl_btn_overviewcanvas(XEvent *event);
void mapctrl_btn_wakeup(XEvent *event);

void center_on_unit(void);

extern int city_workers_color;

#endif  /* FC__MAPCTRL_H */
