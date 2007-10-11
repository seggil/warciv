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

/********************************************************************** 
  Common network interface.
**********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif 
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif
#ifdef HAVE_WINSOCK
#include <winsock.h>
#endif
#ifdef WIN32_NATIVE
#include <windows.h>	/* GetTempPath */
#endif

#include "fciconv.h"
#include "fcintl.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "shared.h"		/* TRUE, FALSE */
#include "support.h"
#include "tadns.h"

#include "netintf.h"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* used by tADNS for adns functions */
static struct dns *dns = NULL;

static int net_lookup_service_id = 0;
static struct hash_table *net_lookup_service_table = NULL;

static int adns_request_id = 0;
static struct hash_table *adns_request_table = NULL;

/* used by net_lookup_service_async */
struct net_lookup_ctx {
  net_lookup_result_callback_t callback;
  void *userdata;
  data_free_func_t datafree;
  union my_sockaddr *addr;
  int adns_id;
  int req_id;
};

/* used by adns functions */
struct adns_ctx {
  adns_result_callback_t callback;
  void *userdata;
  data_free_func_t datafree;
  int req_id;
};

/***************************************************************************
  ...
***************************************************************************/
static void check_init_lookup_tables(void)
{
  if (!net_lookup_service_table) {
    net_lookup_service_table = hash_new(hash_fval_int,
					hash_fcmp_int);
  }
  if (!adns_request_table) {
    adns_request_table = hash_new(hash_fval_int,
				  hash_fcmp_int);
  }
}
/***************************************************************************
  ...
***************************************************************************/
int adns_get_socket_fd(void)
{
  return dns ? dns_get_fd(dns) : -1;
}
/***************************************************************************
  ...
***************************************************************************/
void adns_free(void)
{
  if (dns) {
    dns_fini(dns);
    dns = NULL;
  }
}
/***************************************************************************
  ...
***************************************************************************/
bool adns_is_available(void)
{
  return dns != NULL;
}
/***************************************************************************
  ...
***************************************************************************/
void adns_check_expired(void)
{
  dns_check_expired(dns);
}
/***************************************************************************
  Initialize the asynchronous DNS subsystem. Be sure to add the fd from
  adns_get_socket_fd to some kind of select loop and call adns_input_ready
  when data arrives.
***************************************************************************/
void adns_init(void)
{
  dns = dns_init();
  if (dns == NULL) {
    freelog(LOG_ERROR, 
	    _("Failed to initialize asynchronous DNS resolver."));
  }
}
/***************************************************************************
  ...
***************************************************************************/
static void dns_result_callback(void *data,
				enum dns_query_type qtype,
				const char *hostname,
				const unsigned char *addr,
				size_t addrlen)
{
  struct adns_ctx *ctx = data;

  freelog(LOG_DEBUG, "drc dns_result_callback: data=%p " /*ASYNCDEBUG*/
	  " qtype=%d hostname=\"%s\" addrlen=%d", /*ASYNCDEBUG*/
	  data, qtype, hostname, addrlen); /*ASYNCDEBUG*/
  
  assert(ctx != NULL);
  assert(ctx->callback != NULL);

  freelog(LOG_DEBUG, "drc   calling %p userdata=%p",
	  ctx->callback, ctx->userdata); /*ASYNCDEBUG*/
  if (addrlen <= 0) {
    (*ctx->callback) (NULL, 0, ctx->userdata);
  } else {
    (*ctx->callback) (addr, addrlen, ctx->userdata);
  }

  if (ctx->req_id == -1) {
    /* we were called directly by dns_queue, so don't free ctx! :) */
    ctx->req_id = 0;
  } else {
    freelog(LOG_DEBUG,
	    "drc   hash_delete_entry adsn id=%d", /*ASYNCDEBUG*/
	    ctx->req_id); /*ASYNCDEBUG*/
    hash_delete_entry(adns_request_table, INT_TO_PTR(ctx->req_id));
    freelog(LOG_DEBUG, "drc   free adns_ctx %p", ctx); /*ASYNCDEBUG*/
    free(ctx);
  }
}
/***************************************************************************
  Should be called when there is data waiting to be read on the socket
  returned by adns_get_socket_fd.
***************************************************************************/
void adns_poll(void)
{
  if (!adns_is_available())
    return;
  freelog(LOG_DEBUG, "ap adns_poll"); /*ASYNCDEBUG*/

  dns_poll(dns); /* should result in a call to dns_result_callback */
}
/***************************************************************************
  Queues an asynchronous dns request for the A record for the given
  hostname. Behaves the same as adns_lookup_full.
***************************************************************************/
int adns_lookup(const char *hostname,
		adns_result_callback_t cb,
		void *data,
		data_free_func_t datafree)
{
  return adns_lookup_full(hostname, DNS_A_RECORD, cb, data, datafree);
}
/***************************************************************************
  Queues an asynchronous dns lookup.
  Parameters:
    query_data - the thing to lookup (either name or address).
    query_type - type of dns query.
    cb       - function to call when the result is obtained.
    data     - passed to cb.
    datafree - called on data if this request is cancelled with adns_cancel.
  Returns:
    -1 - an error occured, sorry.
    0  - cb was called directly, datafree not called.
    >0 - the request is in progress, cb will be called in the future and
         the request may be cancelled with adns_cancel.
***************************************************************************/
int adns_lookup_full(const char *query_data,
		     enum dns_query_type query_type,
		     adns_result_callback_t cb,
		     void *data,
		     data_free_func_t datafree)
{
  struct adns_ctx *ctx;
  int ret = 0;

  assert(query_type == DNS_A_RECORD || query_type == DNS_PTR_RECORD);

  freelog(LOG_DEBUG, 
	  "alf adns_lookup_full: query_data=\"%s\"" /*ASYNCDEBUG*/
	  " query_type=%d cb=%p data=%p datafree=%p", /*ASYNCDEBUG*/
	  query_data, query_type, cb, data, datafree); /*ASYNCDEBUG*/

  check_init_lookup_tables();
  assert(dns != NULL);

  ctx = fc_malloc(sizeof(struct adns_ctx));
  freelog(LOG_DEBUG, "alf   new adns_ctx %p", ctx); /*ASYNCDEBUG*/
  ctx->callback = cb;
  ctx->userdata = data;
  ctx->datafree = datafree;
  ctx->req_id = -1;

  /* NB this may call dns_result_callback directly */
  dns_queue(dns, ctx, query_data, query_type, dns_result_callback);

  if (ctx->req_id == 0) {
    /* dns_queue called dns_result_callback directly */
    freelog(LOG_DEBUG, "alf   free adns_ctx %p", ctx); /*ASYNCDEBUG*/
    free(ctx);
    ret = 0;
  } else {
    ctx->req_id = ++adns_request_id;
    freelog(LOG_DEBUG, "alf   hash_insert adns req ctx=%p id=%d", 
	    ctx, ctx->req_id); /*ASYNCDEBUG*/
    hash_insert(adns_request_table, INT_TO_PTR(ctx->req_id), ctx);
    ret = ctx->req_id;
  }
  return ret;
}
/***************************************************************************
  Find the hostname for a given address, asynchronously.
***************************************************************************/
int adns_reverse_lookup(union my_sockaddr *sa,
			adns_result_callback_t cb,
			void *data,
			data_free_func_t datafree)
{
  const unsigned char *addr;
  char buf[64];

  addr = (const unsigned char *) &sa->sockaddr_in.sin_addr;
  my_snprintf(buf, sizeof(buf), "%d.%d.%d.%d.in-addr.arpa",
	      addr[3], addr[2], addr[1], addr[0]);
  return adns_lookup_full(buf, DNS_PTR_RECORD, cb, data, datafree);
}
/***************************************************************************
  Cancels the adns request with the given id. If datafree from adns_lookup
  is not NULL, it will be called on data, otherwise data will be returned
  from this function.
***************************************************************************/
void *adns_cancel(int id)
{
  void *ret = NULL;
  struct adns_ctx *ctx;

  freelog(LOG_DEBUG, "ac adns_cancel: id=%d", id); /*ASYNCDEBUG*/

  check_init_lookup_tables();

  freelog(LOG_DEBUG, "ac   hash_delete_entry adns id=%d", id); /*ASYNCDEBUG*/
  ctx = hash_delete_entry(adns_request_table, INT_TO_PTR(id));
  if (ctx) {
    assert(ctx->req_id == id);
    dns_cancel(dns, ctx);
    if (ctx->datafree) {
      freelog(LOG_DEBUG, "ac   calling datafree %p on %p", 
	      ctx->datafree, ctx->userdata); /*ASYNCDEBUG*/
      (*ctx->datafree) (ctx->userdata);
    } else {
      ret = ctx->userdata;
    }
    freelog(LOG_DEBUG, "ac   free adns_ctx %p", ctx); /*ASYNCDEBUG*/
    free(ctx);
  }
  return ret;
}
/***************************************************************************
  ...
***************************************************************************/
static void nlcfree(void *data)
{
  struct net_lookup_ctx *ctx = data;
  
  freelog(LOG_DEBUG, "nf nlcfree: data=%p", data); /*ASYNCDEBUG*/
           
  if (!ctx)
    return;
  if (ctx->req_id > 0) {
    freelog(LOG_DEBUG, "nf  hash_delete_entry nls id=%d", 
	    ctx->req_id); /*ASYNCDEBUG*/
    hash_delete_entry(net_lookup_service_table, INT_TO_PTR(ctx->req_id));
  }
  if (ctx->addr)
    free(ctx->addr);
  if (ctx->adns_id > 0)
    adns_cancel(ctx->adns_id);
  freelog(LOG_DEBUG, "nf  free net_lookup_ctx %p", ctx); /*ASYNCDEBUG*/
  free(ctx);
}
/***************************************************************************
  ...
***************************************************************************/
static void adns_result_callback(const unsigned char *address,
				 int addrlen, void *data)
{
  struct net_lookup_ctx *ctx = data;
  struct sockaddr_in *sock;

  assert(ctx != NULL);

  freelog(LOG_ERROR, "address=%p addrelen=%d data=%p",
          address, addrlen, data);

  if (address && addrlen > 0) {
    assert(ctx->addr != NULL);
    sock = &ctx->addr->sockaddr_in;
    memcpy(&sock->sin_addr, address, addrlen);
    assert(ctx->callback != NULL);
    (*ctx->callback) (ctx->addr, ctx->userdata);
  } else {
    /* host name could not be resolved */
    assert(ctx->callback != NULL);
    (*ctx->callback) (NULL, ctx->userdata);
  }

  if (ctx->req_id > 0) {
    /* we were called indirectly, so it's ok to free ctx */
    nlcfree(ctx);
  }
}

/***************************************************************************
  Cancels an net_lookup_service request in progress. Behaves analogously
  to adns_cancel.
***************************************************************************/
void *cancel_net_lookup_service(int id)
{
  struct net_lookup_ctx *ctx;
  void *ret = NULL;

  freelog(LOG_DEBUG,
	  "cnls cancel_net_lookup_service id=%d", id); /*ASYNCDEBUG*/
  
  check_init_lookup_tables();

  freelog(LOG_DEBUG, "cnls   hash_delete_entry nlc id=%d", id); /*ASYNCDEBUG*/
  ctx = hash_delete_entry(net_lookup_service_table, INT_TO_PTR(id));
  if (ctx) {
    assert(ctx->req_id == id);
    if (ctx->datafree) {
      freelog(LOG_DEBUG, "cnls   calling datafree %p on %p", 
	      ctx->datafree, ctx->userdata); /*ASYNCDEBUG*/
      (*ctx->datafree) (ctx->userdata);
    } else {
      ret = ctx->userdata;
    }
    nlcfree(ctx);
  }
  return ret;
}
/***************************************************************************
  Just like net_lookup_service, but asynchronous. Behaves analogously to
  adns_lookup. NB cb may be called directly.
***************************************************************************/
int net_lookup_service_async(const char *name, int port,
			     net_lookup_result_callback_t cb,
			     void *data, data_free_func_t datafree)
{
  struct net_lookup_ctx *ctx;
  int adns_id;

  check_init_lookup_tables();

  freelog(LOG_DEBUG, 
	  "nlsa net_lookup_service_async: name=\"%s\" port=%d " /*ASYNCDEBUG*/
	  "cb=%p data=%p datafree=%p", name, port, cb, data, datafree); /*ASYNCDEBUG*/

  ctx = fc_malloc(sizeof(struct net_lookup_ctx));
  freelog(LOG_DEBUG, "nlsa   new net_lookup_ctx %p", ctx); /*ASYNCDEBUG*/
  ctx->callback = cb;
  ctx->userdata = data;
  ctx->datafree = datafree;
  ctx->addr = fc_malloc(sizeof(union my_sockaddr));
  ctx->req_id = 0;
  ctx->adns_id = -1;
  
  if (is_net_service_resolved (name, port, ctx->addr)) {
    freelog(LOG_DEBUG, 
	    "nlsa   is_net_service_resolved return TRUE"); /*ASYNCDEBUG*/
    (*ctx->callback) (ctx->addr, ctx->userdata);
    free(ctx->addr);
    freelog(LOG_DEBUG, "nlsa   free net_lookup_ctx %p", ctx); /*ASYNCDEBUG*/
    free(ctx);
    return 0;
  }
  
  /* this may call adns_result_callback directly */
  adns_id = adns_lookup(name, adns_result_callback,
			ctx, NULL);
  freelog(LOG_DEBUG, "nlsa   got adns_id=%d", adns_id); /*ASYNCDEBUG*/
  
  if (adns_id == 0) {
    /* adns_result_callback called already, no lookup required */
    freelog(LOG_DEBUG, "nlsa   free net_lookup_ctx %p", ctx); /*ASYNCDEBUG*/
    free(ctx->addr);
    free(ctx);
    return 0;
  }

  ctx->adns_id = ctx->adns_id;
  ctx->req_id = ++net_lookup_service_id;
  freelog(LOG_DEBUG, "nlsa   nls req_id = %d", ctx->req_id); /*ASYNCDEBUG*/

  freelog(LOG_DEBUG, "nlsa   hash_insert nls req ctx=%p id=%d", /*ASYNCDEBUG*/
	  ctx, ctx->req_id); /*ASYNCDEBUG*/
  hash_insert(net_lookup_service_table,
	      INT_TO_PTR(ctx->req_id), ctx);
  
  return ctx->req_id;
}
/***************************************************************
  Read from a socket.
***************************************************************/
int my_readsocket(int sock, void *buf, size_t size)
{
#ifdef HAVE_WINSOCK
  return recv(sock, buf, size, 0);
#else
  return read(sock, buf, size);
#endif
}

/***************************************************************
  Write to a socket.
***************************************************************/
int my_writesocket(int sock, const void *buf, size_t size)
{
#ifdef HAVE_WINSOCK
  return send(sock, buf, size, 0);
#else
  return write(sock, buf, size);
#endif
}

/***************************************************************
  Close a socket.
***************************************************************/
void my_closesocket(int sock)
{
#ifdef HAVE_WINSOCK
  closesocket(sock);
#else
  close(sock);
#endif
}

/***************************************************************
  Initialize network stuff.
***************************************************************/
void my_init_network(void)
{
#ifdef HAVE_WINSOCK
  WSADATA wsa;

  if (WSAStartup(MAKEWORD(1, 1), &wsa) != 0) {
    freelog(LOG_ERROR, "no usable WINSOCK.DLL: %s", mystrsocketerror());
  }
#endif

  /* broken pipes are ignored. */
#ifdef HAVE_SIGPIPE
  (void) signal(SIGPIPE, SIG_IGN);
#endif
}

/***************************************************************
  Shutdown network stuff.
***************************************************************/
void my_shutdown_network(void)
{
#ifdef HAVE_WINSOCK
  WSACleanup();
#endif
}

/***************************************************************
  Returns true if errno indicates that the previous socket
  operation would block.
***************************************************************/
bool my_socket_would_block(void)
{
#ifdef HAVE_WINSOCK
  return WSAGetLastError() == WSAEWOULDBLOCK;
#else
  return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}
/***************************************************************
  Returns true if errno indicates that the previous socket
  is in progress.
***************************************************************/
bool my_socket_operation_in_progess(void)
{
#ifdef HAVE_WINSOCK
  return WSAGetLastError() == WSAEINPROGRESS;
#else
  return errno == EINPROGRESS;
#endif
}
/***************************************************************
  Set socket to non-blocking.
***************************************************************/
int my_nonblock(int sockfd)
{
#ifdef NONBLOCKING_SOCKETS
#if defined (HAVE_FCNTL)
  int f_set;

  if ((f_set=fcntl(sockfd, F_GETFL)) == -1) {
    freelog(LOG_ERROR, _("fcntl F_GETFL failed: %s"), mystrsocketerror());
    return -1;
  }

  f_set |= O_NONBLOCK;

  if (fcntl(sockfd, F_SETFL, f_set) == -1) {
    freelog(LOG_ERROR, _("fcntl F_SETFL failed: %s"), mystrsocketerror());
    return -1;
  }
#elif defined (HAVE_IOCTL)
  long value=1;

  if (ioctl(sockfd, FIONBIO, (char*)&value) == -1) {
    freelog(LOG_ERROR, _("ioctl failed: %s"), mystrsocketerror());
    return -1;
  }
#elif defined (HAVE_WINSOCK)
  long one=1;
  if (SOCKET_ERROR == ioctlsocket(sockfd, FIONBIO, &one)) {
    freelog(LOG_ERROR, _("ioctlsocket failed: %s"), mystrsocketerror());
    return -1;
  }
#else
  /* It would be bad, if we got here. */
#error NONBLOCKING_SOCKETS defined, but I do not know how to use them :(
#endif

#else /* i.e. ! NONBLOCKING_SOCKETS */
  freelog(LOG_ERROR, _("my_nonblock called but we don't have "
		       "non blocking sockets on this platform!"));
#endif /* NONBLOCKING_SOCKETS */
  return 0;
}
  
/***************************************************************************
  Returns true if the name is already resolved. Also fills in sockaddr_in
  parts of addr that do no require name resolution.
***************************************************************************/
bool is_net_service_resolved(const char *name, int port,
			     union my_sockaddr *addr)
{
  struct sockaddr_in *sock = &addr->sockaddr_in;

  sock->sin_family = AF_INET;
  sock->sin_port = htons(port);

  if (!name) {
    sock->sin_addr.s_addr = htonl(INADDR_ANY);
    return TRUE;
  }
  
#ifdef HAVE_INET_ATON
  if (inet_aton(name, &sock->sin_addr) != 0) {
    return TRUE;
  }
#else
  if ((sock->sin_addr.s_addr = inet_addr(name)) != INADDR_NONE) {
    return TRUE;
  }
#endif
  
  return FALSE;
}
 
/***************************************************************************
  Look up the service at hostname:port and fill in *sa.
***************************************************************************/
bool net_lookup_service(const char *name, int port, union my_sockaddr *addr)
{
  struct hostent *hp;
  struct sockaddr_in *sock = &addr->sockaddr_in;
  
  if (is_net_service_resolved(name, port, addr))
    return TRUE;
  
  hp = gethostbyname(name);
  if (!hp || hp->h_addrtype != AF_INET) {
    return FALSE;
  }
  
  memcpy(&sock->sin_addr, hp->h_addr, hp->h_length);
  return TRUE;
}
 
/*************************************************************************
  Writes buf to socket and returns the response in an fz_FILE.
  Use only on blocking sockets.
*************************************************************************/
fz_FILE *my_querysocket(int sock, void *buf, size_t size)
{
  FILE *fp;
  
#ifdef HAVE_FDOPEN
  fp = fdopen(sock, "r+b");
  if (fwrite(buf, 1, size, fp) != size) {
    die("socket %d: write error", sock);
  }
  fflush(fp);
  
  /* we don't use my_closesocket on sock here since when fp is closed
   * sock will also be closed. fdopen doesn't dup the socket descriptor. */
#else
  {
    char tmp[4096];
    int n;
    
    fp = my_tmpfile();
    
    if (fp == NULL) {
      return NULL;
    }
    
    my_writesocket(sock, buf, size);

    while ((n = my_readsocket(sock, tmp, sizeof(tmp))) > 0) {
      if (fwrite(tmp, 1, n, fp) != n) {
	die("socket %d: write error", sock);
      }
    }
    fflush(fp);
    
    my_closesocket(sock);
    
    rewind(fp);
  }
#endif
  
  return fz_from_stream(fp);
}
 
/*************************************************************************
  Returns a valid httpd server and port, plus the path to the resource
  at the url location.
*************************************************************************/
const char *my_lookup_httpd(char *server, int *port, const char *url)
{
  const char *purl, *str, *ppath, *pport;
  
  if ((purl = getenv("http_proxy"))) {
    if (strncmp(purl, "http://", strlen("http://")) != 0) {
      return NULL;
    }
    str = purl;
  } else {
    if (strncmp(url, "http://", strlen("http://")) != 0) {
      return NULL;
    }
    str = url;
  }
  
  str += strlen("http://");
  
  pport = strchr(str, ':');
  ppath = strchr(str, '/');
  
  /* snarf server. */
  server[0] = '\0';

  if (pport) {
    strncat(server, str, MIN(MAX_LEN_ADDR, pport-str));
  } else {
    if (ppath) {
      strncat(server, str, MIN(MAX_LEN_ADDR, ppath-str));
    } else {
      strncat(server, str, MAX_LEN_ADDR);
    }
  }

  /* snarf port. */
  if (!pport || sscanf(pport+1, "%d", port) != 1) {
    *port = 80;
  }

  /* snarf path. */
  if (!ppath) {
    ppath = "/";
  }

  return (purl ? url : ppath);
}

/*************************************************************************
  Returns TRUE if ch is an unreserved ASCII character.
*************************************************************************/
static bool is_url_safe(unsigned ch)
{
  const char *unreserved = "-_.!~*'|";

  if ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z') || (ch>='0' && ch<='9')) {
    return TRUE;
  } else {
    return (strchr(unreserved, ch) != NULL);
  }
}

/***************************************************************
  URL-encode a string as per RFC 2396.
  Should work for all ASCII based charsets: including UTF-8.
***************************************************************/
const char *my_url_encode(const char *txt)
{
  static char buf[2048];
  unsigned ch;
  char *ptr;

  /* in a worst case scenario every character needs "% HEX HEX" encoding. */
  if (sizeof(buf) <= (3*strlen(txt))) {
    return "";
  }
  
  for (ptr = buf; *txt != '\0'; txt++) {
    ch = (unsigned char) *txt;

    if (is_url_safe(ch)) {
      *ptr++ = *txt;
    } else if (ch == ' ') {
      *ptr++ = '+';
    } else {
      sprintf(ptr, "%%%2.2X", ch);
      ptr += 3;
    }
  }
  *ptr++ = '\0';

  return buf;
}

/************************************************************************** 
  Finds the next (lowest) free port.
**************************************************************************/ 
int find_next_free_port(int starting_port)
{
  int port, s = socket(AF_INET, SOCK_STREAM, 0);

  for (port = starting_port;; port++) {
    union my_sockaddr tmp;
    struct sockaddr_in *sock = &tmp.sockaddr_in;

    memset(&tmp, 0, sizeof(tmp));

    sock->sin_family = AF_INET;
    sock->sin_port = htons(port);
    sock->sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(s, &tmp.sockaddr, sizeof(tmp.sockaddr)) == 0) {
      break;
    }
  }

  my_closesocket(s);
  
  return port;
}

/************************************************************************** 
  For debugging mostly...
**************************************************************************/ 
int my_errno(void)
{
#ifdef HAVE_WINSOCK
  return WSAGetLastError();
#else
  return errno;
#endif
}
