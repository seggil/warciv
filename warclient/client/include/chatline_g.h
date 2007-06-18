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
#ifndef FC__CHATLINE_G_H
#define FC__CHATLINE_G_H

#include "chatline_common.h"

void real_append_output_window(const char *astring, int conn_id);
void log_output_window(void);
void clear_output_window(void);

struct section_file;

void secfile_save_chatline_colors (struct section_file *sf);
void secfile_load_chatline_colors (struct section_file *sf);

struct section_file;

void secfile_save_chatline_colors (struct section_file *sf);
void secfile_load_chatline_colors (struct section_file *sf);

#endif  /* FC__CHATLINE_G_H */
