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
#ifndef WC_CLIENT_GUI_MESSAGEDLG_H
#define WC_CLIENT_GUI_MESSAGEDLG_H

void apply_message_options(GtkWidget *widget);
void refresh_message_options(GtkWidget *widget);
void reset_message_options(GtkWidget *widget);
void reload_message_options(GtkWidget *widget, struct section_file *sf);

GtkWidget *create_messages_configuration(void);

#endif  /* WC_CLIENT_GUI_MESSAGEDLG_H */
