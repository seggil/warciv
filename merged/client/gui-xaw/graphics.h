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
#ifndef FC__GRAPHICS_H
#define FC__GRAPHICS_H

#include <X11/Xlib.h>

#include "graphics_g.h"

struct Sprite {
  Pixmap pixmap, mask;
  int width, height, ncols;
  unsigned long *pcolorarray;
  int has_mask;
};

struct canvas {
  Pixmap pixmap;
};

Pixmap create_overlay_unit(int i);

extern struct Sprite *intro_gfx_sprite;
extern struct Sprite *radar_gfx_sprite;
extern Cursor         goto_cursor;
extern Cursor         drop_cursor;
extern Cursor         nuke_cursor;
extern Cursor         patrol_cursor;

#endif  /* FC__GRAPHICS_H */
