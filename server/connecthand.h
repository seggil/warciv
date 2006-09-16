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
#ifndef FC__CONNECTHAND_H
#define FC__CONNECTHAND_H

#include "shared.h"		/* bool type */

#include "fc_types.h"

enum action_type {
	ACTION_BAN = 0,
	ACTION_GIVE_NONE,
	ACTION_GIVE_INFO,
	ACTION_GIVE_CTRL,
	ACTION_GIVE_ADMIN,
	ACTION_GIVE_HACK,
  
	NUM_ACTIONS
};

enum action_pattern_type {
  APT_ADDRESS = 0,
  APT_HOSTNAME,
  APT_USERNAME,

  NUM_ACTION_PATTERN_TYPES
};

struct user_action {
  char *pattern;
  enum action_pattern_type type;
  enum action_type action;	
};

#define SPECLIST_TAG user_action
#define SPECLIST_TYPE struct user_action
#include "speclist.h"

#define user_action_list_iterate(alist, pitem) \
    TYPED_LIST_ITERATE (struct user_action, alist, pitem)
#define user_action_list_iterate_end LIST_ITERATE_END

extern struct user_action_list on_connect_user_actions;
extern char *user_action_strs[NUM_ACTIONS];
extern char *user_action_pattern_type_strs[NUM_ACTION_PATTERN_TYPES];

struct connection;
struct conn_list;
struct packet_authentication_reply;
struct packet_login_request;
struct packet_server_join_req;

void establish_new_connection(struct connection *pconn);
void reject_new_connection(const char *msg, struct connection *pconn);

bool handle_login_request(struct connection *pconn,
                          struct packet_server_join_req *req);

void lost_connection_to_client(struct connection *pconn);

void send_conn_info(struct conn_list *src, struct conn_list *dest);
void send_conn_info_remove(struct conn_list *src, struct conn_list *dest);

bool attach_connection_to_player(struct connection *pconn, 
                                 struct player *pplayer);
bool unattach_connection_from_player(struct connection *pconn);
bool is_banned (char *username, struct connection *pconn);
void clear_all_on_connect_user_actions (void);

#endif /* FC__CONNECTHAND_H */
