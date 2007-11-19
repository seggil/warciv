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
  ACTION_GIVE_OBSERVER,
  ACTION_GIVE_BASIC,
  ACTION_GIVE_CTRL,
  ACTION_GIVE_ADMIN,
  ACTION_GIVE_HACK,
  
  NUM_ACTION_TYPES,
};

struct conn_pattern;

struct user_action {
  struct conn_pattern *conpat;
  enum action_type action;	
};

struct user_action *user_action_new(const char *pattern, int type,
                                    int action);
void user_action_free(struct user_action *pua);
int user_action_as_str(struct user_action *pua,
                       char *buf, int buflen);

#define SPECLIST_TAG user_action
#define SPECLIST_TYPE struct user_action
#include "speclist.h"

#define user_action_list_iterate(alist, pitem) \
    TYPED_LIST_ITERATE (struct user_action, alist, pitem)
#define user_action_list_iterate_end LIST_ITERATE_END

extern struct user_action_list on_connect_user_actions;
extern char *user_action_type_strs[NUM_ACTION_TYPES];

struct connection;
struct conn_list;
struct packet_authentication_reply;
struct packet_login_request;
struct packet_server_join_req;

bool can_control_a_player(struct connection *pconn, bool message);

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

void server_assign_nation(struct player *pplayer,
                          Nation_Type_id nation_no,
                          const char *name,
                          bool is_male,
                          int city_style);
bool server_assign_random_nation(struct player *pplayer);

void grant_access_level(struct connection *pconn);
#endif /* FC__CONNECTHAND_H */
