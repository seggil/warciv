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
#ifndef WC_SERVER_HANDCHAT_H
#define WC_SERVER_HANDCHAT_H

#define ALLIESCHAT_COMMAND_PREFIX '.'

#include "connection.hh"

void handle_chat_msg_req(connection_t *pconn, char *message);
bool conn_is_ignored(connection_t *pconn, connection_t *dest);

#endif  /* WC_SERVER_HANDCHAT_H */
