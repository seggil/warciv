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
#ifndef WC_SERVER_CONNECTHAND_H
#define WC_SERVER_CONNECTHAND_H

#include "shared.h"             /* bool type */

#include "wc_types.h"

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

extern struct user_action_list *on_connect_user_actions;
extern char *user_action_type_strs[NUM_ACTION_TYPES];

struct connection;
struct conn_list;
struct packet_authentication_reply;
struct packet_login_request;
struct packet_server_join_req;

bool can_control_a_player(struct connection *pconn, bool message);

bool receive_ip(struct connection *pconn, const char *ipaddr);
bool receive_hostname(struct connection *pcon, const char *addr);
bool receive_username(struct connection *pconn, const char *username);

void establish_new_connection(struct connection *pconn);
void reject_new_connection(const char *msg, struct connection *pconn);

bool handle_login_request(struct connection *pconn,
                          struct packet_server_join_req *req);

void lost_connection_to_client(struct connection *pconn);

void send_conn_info(struct conn_list *src, struct conn_list *dest);
void send_conn_info_remove(struct conn_list *src, struct conn_list *dest);

bool attach_connection_to_player(struct connection *pconn,
                                 player_t *pplayer);
bool unattach_connection_from_player(struct connection *pconn);
void clear_all_on_connect_user_actions(void);
void restore_access_level(struct connection *pconn);

void server_assign_nation(player_t *pplayer,
                          Nation_Type_id nation_no,
                          const char *name,
                          bool is_male,
                          int city_style);
bool server_assign_random_nation(player_t *pplayer);

void conn_reset_idle_time(struct connection *pconn);
void check_idle_connections(void);

#endif  /* WC_SERVER_CONNECTHAND_H */
