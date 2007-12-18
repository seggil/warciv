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

#ifndef FC__NETINTF_H
#define FC__NETINTF_H

/********************************************************************** 
  Common network interface.
***********************************************************************/

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef WIN32_NATIVE
# include <winsock2.h>
#endif

#include "ioz.h"
#include "shared.h"		/* bool type */
#include "tadns.h"

#ifdef FD_ZERO
#define MY_FD_ZERO FD_ZERO
#else
#define MY_FD_ZERO(p) memset((void *)(p), 0, sizeof(*(p)))
#endif

union my_sockaddr {
  struct sockaddr sockaddr;
  struct sockaddr_in sockaddr_in;
};

int my_readsocket(int sock, void *buf, size_t size);
int my_writesocket(int sock, const void *buf, size_t size); 
void my_closesocket(int sock);
void my_init_network(void);         
void my_shutdown_network(void);
bool my_socket_would_block(void);
bool my_socket_operation_in_progess(void);
int my_nonblock(int sockfd);
int my_errno(void);

bool is_net_service_resolved(const char *name, int port,
                             union my_sockaddr *addr);
bool net_lookup_service(const char *name, int port, 
                        union my_sockaddr *addr);
fz_FILE *my_querysocket(int sock, void *buf, size_t size);
int find_next_free_port(int starting_port);

const char *my_lookup_httpd(char *server, int *port, const char *url);
const char *my_url_encode(const char *txt);

/***************************************************************
  The callback function that is called with the results of adns
  requests. Parameters:
     addr - For DNS_A_RECORD queries, the internet address of
            the queried hostname (i.e. the bytes of
            sockaddr_in.sin_addr). For DNS_PTR_RECORD queries,
            the hostname corresponding to the queried address.
     addrlen - byte length of addr.
     data - The context data passed in to the adns lookup
            function.
  If the query fails, addr is set to NULL and addrlen to 0.
***************************************************************/
typedef void (*adns_result_callback_t) (const unsigned char *addr,
                                        int addrlen,
                                        void *data);

int adns_lookup_full (const char *query_data,
                      enum dns_query_type query_type,
                      adns_result_callback_t cb,
                      void *data,
                      data_free_func_t datafree);
int adns_lookup (const char *name,
                 adns_result_callback_t cb,
                 void *data,
                 data_free_func_t datafree);
int adns_reverse_lookup (union my_sockaddr *sa,
                         adns_result_callback_t cb,
                         void *data,
                         data_free_func_t datafree);
void *adns_cancel (int id);
void adns_init (void);
void adns_free (void);
bool adns_is_available (void);
int adns_get_socket_fd (void);
void adns_poll (void);
void adns_check_expired (void);

union my_sockaddr;
typedef void (*net_lookup_result_callback_t) (union my_sockaddr *addr,
                                              void *data);
int net_lookup_service_async (const char *name,
                              int port,
                              net_lookup_result_callback_t cb,
                              void *data,
                              data_free_func_t datafree);
void *cancel_net_lookup_service (int id);

#endif  /* FC__NETINTF_H */
