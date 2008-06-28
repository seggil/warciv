/*
 * Copyright (c) 2004-2005 Sergey Lyubka <valenok@gmail.com>
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Sergey Lyubka wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef WIN32_NATIVE
#include <winsock2.h>
#include <limits.h>
#include <stdint.h>
#include <ws2tcpip.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "fciconv.h"
#include "fcintl.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "shared.h"
#include "support.h"

#include "tadns.h"

#define DNS_MAX           1025      /* Maximum host name */
#define DNS_PACKET_LEN    2048 /* Buffer size for DNS packet */

/* The DNS name query string that is made to
 * try and do a reverse lookup on the localhost
 * address 127.0.0.1. */
#define LOCALHOST_RLQUERY "1.0.0.127.in-addr.arpa"

/* Instead of sending a full reverse lookup query
 * for LOCALHOST_RLQUERY, just return the following
 * string as the hostname. */
#define LOCALHOST_HOSTNAME "localhost"

struct dns;

#define QUERY_MEMORY_GUARD 0xbedabb1e

/* User query. Holds mapping from application-level ID to DNS transaction id,
 * and user defined callback function.
 */
struct query {
  int guard;
  int refcount;
  time_t expire;                /* Time when this query expire */
  uint16_t tid;                 /* UDP DNS transaction ID, big endian */
  uint16_t qtype;               /* Query type */
  char name[DNS_MAX];           /* Host name */
  dns_callback_t callback;      /* User callback routine */
  void *userdata;               /* Application context */
  unsigned char addr[DNS_MAX];  /* Host address */
  size_t addrlen;               /* Address length */
};

/* Resolver descriptor. */
struct dns {
  int sock;                     /* UDP socket used for queries */
  struct sockaddr_in sa;        /* DNS server socket address */
  uint16_t tid;                 /* Latest tid used */

  struct hash_table *active;
  struct hash_table *cache;

  char buf[4096];
  size_t buflen;
};

/* DNS network packet */
struct header {
  uint16_t tid;                 /* Transaction ID */
  uint16_t flags;               /* See RFC 1035 */
  uint16_t nqueries;            /* Questions */
  uint16_t nanswers;            /* Answers */
  uint16_t nauth;               /* Authority PRs */
  uint16_t nother;              /* Other PRs */
  unsigned char data[1];        /* Data, variable length */
};

/**************************************************************************
  Return UDP socket used by a resolver
**************************************************************************/
int dns_get_fd(struct dns *dns)
{
  return dns->sock;
}

/**************************************************************************
  Fetch name from DNS packet
**************************************************************************/
static void fetch(const uint8_t *pkt,
                  int pktsiz,
                  const uint8_t *s,
                  char *dst,
                  int dstlen)
{
  const uint8_t *e = pkt + pktsiz;
  int j, i = 0, n = 0;


  while (*s != 0 && s < e) {
    if (n > 0)
      dst[i++] = '.';

    if (i >= dstlen)
      break;

    if ((n = *s++) == 0xc0) {
      s = pkt + *s;             /* New offset */
      n = 0;
    } else {
      for (j = 0; j < n && i < dstlen; j++)
        dst[i++] = *s++;
    }
  }

  dst[i] = '\0';
}

/**************************************************************************
  Find what DNS server to use. Return 0 if OK, -1 if error
**************************************************************************/
static int getdnsip(struct dns *dns)
{
  int ret = -1;

#ifdef WIN32_NATIVE
  int i;
  long int err;
  DWORD tlen;
  HKEY hKey, hSub;
  char subkey[512], value[128], *key =
    "SYSTEM\\ControlSet001\\Services\\Tcpip\\Parameters\\Interfaces";

  if ((err = RegOpenKeyEx(HKEY_LOCAL_MACHINE, key, 0, KEY_ALL_ACCESS, &hKey)) 
      != ERROR_SUCCESS) {
    freelog(LOG_ERROR, "Cannot open registry key %s: %d", key, err);
  } else {
    for (i = 0, tlen =  sizeof(subkey);
         RegEnumKeyEx(hKey, i, subkey, &tlen, NULL, NULL, NULL, NULL) == 
	   ERROR_SUCCESS;
         i++, tlen =  sizeof(subkey)) {
      DWORD type, len = sizeof(value);
      if (RegOpenKeyEx(hKey, subkey, 0, KEY_ALL_ACCESS, &hSub) 
	  == ERROR_SUCCESS
          && (RegQueryValueEx(hSub, "DhcpNameServer", 0,
                              &type, value, &len) == ERROR_SUCCESS
              || RegQueryValueEx(hSub, "DhcpNameServer", 0,
                                 &type, value, &len) == ERROR_SUCCESS))
	{
	  dns->sa.sin_addr.s_addr = inet_addr(value);
	  ret = 0;
	  RegCloseKey(hSub);
	  break;
	}
    }
    RegCloseKey(hKey);
  }
#else
  FILE *fp;
  char line[512];
  int a, b, c, d;

  if ((fp = fopen("/etc/resolv.conf", "r")) == NULL) {
    ret = -1;
  } else {
    /* Try to figure out what DNS server to use */
    while (fgets(line, sizeof(line), fp) != NULL) {
      if (sscanf(line, "nameserver %d.%d.%d.%d", &a, &b, &c, &d) == 4) {
        dns->sa.sin_addr.s_addr = htonl(a << 24 | b << 16 | c << 8 | d);
        ret = 0;
        break;
      }
    }
    fclose(fp);
  }
#endif /* WIN32_NATIVE */

  return ret;
}

/**************************************************************************
  ...
**************************************************************************/
struct dns *dns_new(void)
{
  struct dns *dns;
  int rcvbufsiz = 64 * 1024;
  uint32_t seed;

  dns = fc_calloc(1, sizeof(struct dns));
  dns->sock = -1;
  
  if ((dns->sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    freelog(LOG_ERROR, _("Failed to create UDP socket: %s"),
            mystrsocketerror(mysocketerrno()));
    goto FAILED;
  }
  
  if (my_set_nonblock(dns->sock) == -1) {
    freelog(LOG_ERROR, _("Failed to set non-blocking mode on UDP socket: %s"),
	    mystrsocketerror(mysocketerrno()));
    goto FAILED;
  }
 
  if (getdnsip(dns) == -1) {
    freelog(LOG_ERROR, _("Could not find address of DNS resolver host."));
    goto FAILED;
  }

  dns->sa.sin_family = AF_INET;
  dns->sa.sin_port = htons(53);

  /* Increase the receive buffer */
  if (-1 == setsockopt(dns->sock, SOL_SOCKET, SO_RCVBUF,
                       (char *) &rcvbufsiz, sizeof(rcvbufsiz)))
  {
    freelog (LOG_ERROR, _("Failed to set UDP socket receive buffer size:"
                          " setsockopt: %s"),
             mystrsocketerror(mysocketerrno()));
    goto FAILED;
  }
  
  dns->active = hash_new(hash_fval_uint16_t, hash_fcmp_uint16_t);
  dns->cache = hash_new(hash_fval_string, hash_fcmp_string_ci);

  /* Not cryptographically strong, but good enough here for now. */
  seed = (uint32_t) time(NULL);
  dns->tid = hash_fval_uint32_t(UINT32_T_TO_PTR(seed), 0xffffffff);
  
  return dns;
  
FAILED:
  dns_destroy(dns);  
  return NULL;
}

/**************************************************************************
  ...
**************************************************************************/
static void destroy_query(struct dns *dns, struct query *query)
{
  freelog(LOG_DEBUG, "dq destroy_query query=%p", query);

  if (!query) {
    return;
  }
  if (query->guard != QUERY_MEMORY_GUARD) {
    freelog(LOG_DEBUG, "dq called on %p more than once", query);
    return;
  }

  if (query->tid > 0) {
    freelog(LOG_DEBUG, "dq removing tid=%x from active table", query->tid);
    hash_delete_entry(dns->active, UINT16_T_TO_PTR(query->tid));
    query->tid = 0;
  }

  if (query->refcount > 0) {
    freelog(LOG_DEBUG, "dq not destroying query %p yet, refcount=%d",
            query, query->refcount);
    query->refcount--;
    return;
  }

  if (query->name[0] != '\0') {
    void *key;
    freelog(LOG_DEBUG, "dq removing name=\"%s\" from cache", query->name);
    hash_delete_entry_full(dns->cache, query->name, &key);
    if (key) {
      free(key);
    }
    query->name[0] = '\0';
  }

  freelog(LOG_DEBUG, "dq freeing query %p", query);

  memset(query, 0, sizeof(struct query));
  free(query);
}

/**************************************************************************
  Find host in host cache.
**************************************************************************/
static struct query *find_cached_query(struct dns *dns,
                                       enum dns_query_type qtype,
                                       const char *name)
{
  struct query *q;
  freelog(LOG_DEBUG, "fcq find_cached_query \"%s\"", name);

#ifdef DEBUG
  freelog(LOG_DEBUG, "fcq cache contains:");
  freelog(LOG_DEBUG, "fcq entries=%d", hash_num_entries(dns->cache));
  freelog(LOG_DEBUG, "fcq buckets=%d", hash_num_buckets(dns->cache));
  freelog(LOG_DEBUG, "fcq deleted=%d", hash_num_deleted(dns->cache));

  hash_iterate(dns->cache, char *, name, struct query *, pq) {
    freelog(LOG_DEBUG, "fcq \"%s\" --> query %p", name, pq);
  } hash_iterate_end;
#endif
  
  q = hash_lookup_data(dns->cache, name);
  freelog(LOG_DEBUG, "fcq %sfound query=%p", q ? "" : "NOT ", q);
  return q;
}

/**************************************************************************
  ...
**************************************************************************/
static struct query *find_active_query(struct dns *dns, uint16_t tid)
{
  return hash_lookup_data(dns->active, UINT16_T_TO_PTR(tid));
}

/**************************************************************************
  User wants to cancel query
**************************************************************************/
void dns_cancel(struct dns *dns, const void *userdata)
{
  freelog(LOG_DEBUG, "dns_cancel dns=%p userdata=%p", dns, userdata);

  hash_iterate(dns->active, void *, key, struct query *, query) {
    if (query->userdata == userdata) {
      destroy_query(dns, query);
      break;
    }
  } hash_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
static void call_user(struct dns *dns, struct query *query)
{
  freelog(LOG_DEBUG, "cu call_user query %p tid=%x name=\"%s\"",
          query, query->tid, query->name);

  assert(query->callback != NULL);

  (*query->callback) (query->addr, query->addrlen,
                      query->name, query->qtype, query->userdata);

  freelog(LOG_DEBUG, "cu returned from callback");
}

/**************************************************************************
  ...
**************************************************************************/
static void parse_udp(struct dns *dns)
{
  struct header *header;
  const unsigned char *p, *e;
  struct query *q;
  uint32_t ttl;
  uint16_t type, qtype, flags;
  char name[1025];
  int dlen, nlen, rcode;
  bool found = FALSE;

  freelog(LOG_DEBUG, "pu parse_udp");
  
  /* We sent 1 query. The response must also have a query count of 1. */
  header = (struct header *) dns->buf;
  if (ntohs(header->nqueries) != 1) {
    freelog(LOG_DEBUG, "pu ignoring packet, nqueries is not 1 (it is %d)",
            header->nqueries);
    return;
  }

  /* Return if we did not send that query */
  if ((q = find_active_query(dns, header->tid)) == NULL) {
    freelog(LOG_DEBUG, "pu ignoring packet, unknown tid=%x", header->tid);
    return;
  }

  /* Check respone code */
  flags = ntohs(header->flags);
  freelog(LOG_DEBUG, "pu header->flags=0x%04x flags=0x%04x",
          header->flags, flags);
  rcode = flags & 0x000f;
  freelog(LOG_DEBUG, "pu rcode=%d", rcode);
  
  if (rcode == 1) {
    freelog(LOG_ERROR, _("DNS query failed: query format error."));
    goto QUERY_FAILED;
  }

  if (rcode == 2) {
    freelog(LOG_ERROR, _("DNS query failed: server failure "
                         "(error code %d)."), rcode);
    goto QUERY_FAILED;
  }

  if (rcode == 3) {
    freelog(LOG_ERROR, _("DNS query failed: name error, no such domain."));
    goto QUERY_FAILED;
  }

  if (rcode != 0) {
    freelog(LOG_ERROR, _("Unknown DNS query failure (error code %d)."),
            rcode);
    goto QUERY_FAILED;
  }

  /* Received 0 answers */
  if (header->nanswers == 0) {
    freelog(LOG_DEBUG, "pu got 0 answers");
    goto QUERY_FAILED;
  }

  /* Skip host name */
  e = (const unsigned char *)(dns->buf + dns->buflen);
  p = header->data;
  for (nlen = 0; p < e && *p != '\0'; p++) {
    nlen++;
  }

  qtype = ntohs(*(uint16_t *) (p + 1));

  /* We sent query class 1, query type 1 */
  if (p + 5 > e || qtype != q->qtype) {
    freelog(LOG_DEBUG, "pu returning on class/query mismatch: "
            "p+5=%p e=%p qtype=%d q->qtype=%d",
            p+5, e, qtype, q->qtype);
    goto QUERY_FAILED;
  }

  /* Go to the first answer section */
  p += 5;

  /* Loop through the answers, we want A type answer */
  found = FALSE;
  while (p + 12 < e) {

    /* Skip possible name in CNAME answer */
    if (*p != 0xc0) {
      while (*p && p + 12 < e) {
        p++;
      }
      p--;
    }

    type = ntohs(((uint16_t *) p)[1]);

    freelog(LOG_DEBUG, "pu got answer type=%d", type);
    if (type == 5) {
      /* CNAME answer. shift to the next section */
      dlen = ntohs(((uint16_t *) p)[5]);
      p += 12 + dlen;
    } else if (type == q->qtype) {
      found = TRUE;
      break;
    } else {
      break;
    }
  }

  if (found && p + 12 < e) {
    dlen = ntohs(((uint16_t *) p)[5]);
    p += 12;

    if (p + dlen > e) {
      return;
    }
    
    ttl = ntohl(*((uint32_t *) (p - 6)));
    q->expire = time(NULL) + (time_t) ttl;
    if (q->expire < 0) {
      q->expire = (time_t) INT_MAX;
    }
    freelog(LOG_DEBUG, "pu got ttl=%u, set expire=%ld", 
            ttl, q->expire);
    
    if (q->qtype == DNS_MX_RECORD) {
      /* Skip 2 byte preference field */
      fetch((const unsigned char *) (dns->buf),
            dns->buflen, p + 2, name, sizeof(name));
      p = (const unsigned char *) name;
      dlen = strlen(name) + 1;
    } else if (q->qtype == DNS_PTR_RECORD) {
      freelog(LOG_DEBUG, "pu extracting DNS_PTR_RECORD");
      fetch((const unsigned char *) (dns->buf),
            dns->buflen, p, name, sizeof(name));
      p = (const unsigned char *) name;
      dlen = strlen(name) + 1;
      freelog(LOG_DEBUG, "pu p=\"%s\" dlen=\"%d\"", p, dlen);
    }

    q->addrlen = dlen;
    if (q->addrlen > sizeof(q->addr)) {
      q->addrlen = sizeof(q->addr);
    }
    memcpy(q->addr, p, q->addrlen);

    /* Move query to cache */
    if (q->name[0] != '\0') {
      freelog(LOG_DEBUG, "pu caching \"%s\" with query=%p", q->name, q);
      hash_insert(dns->cache, mystrdup(q->name), q);
      q->refcount++;
    }

    call_user(dns, q);
  }
  return;

QUERY_FAILED:
  assert(q != NULL);
  q->addrlen = 0;
  call_user(dns, q);
  destroy_query(dns, q);
}

/**************************************************************************
  Check for packets on dns udp socket.
**************************************************************************/
void dns_poll(struct dns *dns)
{
  struct sockaddr_in sa;
  socklen_t len = sizeof(sa);
  int nb;
  long err_no;

  freelog(LOG_DEBUG, "dp dns_poll");

  /* Check our socket for new stuff */
  nb = recvfrom(dns->sock, dns->buf + dns->buflen,
                sizeof(dns->buf) - dns->buflen, 0,
                (struct sockaddr *) &sa, &len);
  err_no = mysocketerrno();
  freelog(LOG_DEBUG, "dp recvfrom got nb=%d, from %s",
          nb, inet_ntoa(sa.sin_addr));

  if (memcmp(&sa.sin_addr, &dns->sa.sin_addr, sizeof(sa.sin_addr))) {
    freelog(LOG_VERBOSE, _("Ignoring UDP packet from %s (not from DNS "
                           "resolver %s)."),
            inet_ntoa(sa.sin_addr), inet_ntoa(dns->sa.sin_addr));
    return;
  }
  
  if (nb < 0) {
    if (my_socket_would_block(err_no)
        || my_socket_operation_in_progess(err_no)) {
      freelog(LOG_DEBUG, "dp recvfrom would block");
    } else {
      freelog(LOG_ERROR, _("Failed reading DNS response: recvfrom: %s"),
              mystrsocketerror(mysocketerrno()));
    }
  }
  
  if (nb > 0) {
    dns->buflen += nb;
    if (dns->buflen > sizeof(struct header)) {
      parse_udp(dns);
    } else {
      freelog(LOG_VERBOSE, 
	      _("Ignoring UDP packet since it is smaller "
		"than the size of a DNS packet header (%lu < %lu)."),
	      (long unsigned) dns->buflen, 
	      (long unsigned) sizeof(struct header));
    }
  }

  dns->buflen = 0;
  
}

/**************************************************************************
  Cleanup
**************************************************************************/
void dns_destroy(struct dns *dns)
{
  const void *q;
  void *key;

  if (!dns) {
    return;
  }

  if (dns->sock != -1) {
    my_closesocket(dns->sock);
    dns->sock = -1;
  }

  if (dns->active) {
    while ((q = hash_key_by_number(dns->active, 1))) {
      hash_delete_entry(dns->active, q);
      destroy_query(dns, (struct query *) q);
    }
    hash_free(dns->active);
    dns->active = NULL;
  }

  if (dns->cache) {
    while ((q = hash_key_by_number(dns->cache, 1))) {
      hash_delete_entry_full(dns->cache, q, &key);
      if (key) {
        free(key);
      }
      destroy_query(dns, (struct query *) q);
    }
    hash_free(dns->cache);
    dns->cache = NULL;
  }

  free(dns);
}

/**************************************************************************
  Queue the resolution
**************************************************************************/
void dns_queue(struct dns *dns,
               const char *name,
               enum dns_query_type qtype,
               dns_callback_t callback,
               void *userdata)
{
  struct query *query;
  struct header *header;
  int i, n, name_len, nb;
  char pkt[DNS_PACKET_LEN], *p;
  const char *s;
  time_t now = time(NULL);
  unsigned int hv;

  freelog(LOG_DEBUG, "dq dns_queue dns=%p userdata=%p name=\"%s\""
          " qtype=%d callback=%p", dns, userdata, name, qtype,
          callback);
  
  /* Search the cache first */
  if ((query = find_cached_query(dns, qtype, name)) != NULL) {
    freelog(LOG_DEBUG, "dq found cached query for \"%s\": query %p",
            name, query);
    query->callback = callback;
    query->userdata = userdata;
    call_user(dns, query);
    if (query->expire < now) {
      destroy_query(dns, query);
    }
    return;
  }

  /* Allocate new query */
  query = fc_calloc(1, sizeof(struct query));

  freelog(LOG_DEBUG, "dq new query %p", query);

  query->guard = QUERY_MEMORY_GUARD;
  query->callback = callback;
  query->userdata = userdata;
  query->qtype = (uint16_t) qtype;
  hv = hash_fval_uint16_t(UINT16_T_TO_PTR(dns->tid), 0xffff);
  query->tid = (uint16_t) hv;
  dns->tid = query->tid;
  query->expire = now + DNS_QUERY_TIMEOUT;

  /* copy over name, converted to lower case */
  for (p = query->name, i = 0;
       name[i] != '\0' && p < query->name + sizeof(query->name) - 1;
       i++, p++)
  {
    *p = tolower(name[i]);
  }
  *p = '\0';

  /* Don't bother asking the resolver for reverse look-up
   * if the query is for 127.0.0.1. */
  if (qtype == DNS_PTR_RECORD
      && 0 == strcmp(name, LOCALHOST_RLQUERY)) {
    size_t len;

    /* On some strange platforms, strlen does not
     * return a size_t apparently. */
    len = (size_t) strlen(LOCALHOST_HOSTNAME) + 1;

    len = MIN(len, sizeof(query->addr) - 1);
    memcpy(query->addr, LOCALHOST_HOSTNAME, len);
    query->addr[len - 1] = 0;
    query->addrlen = len;

    call_user(dns, query);
    destroy_query(dns, query);
    return;
  }

  /* Prepare DNS packet header */
  header = (struct header *) pkt;
  memset(header, 0, sizeof(struct header));

  header->tid = query->tid;
  header->flags = htons(0x0100);  /* Set recursion desired bit */
  header->nqueries = htons(1); /* Just one query */

  /* Encode DNS name */

  name_len = strlen(name);
  p = (char *) &header->data;   /* For encoding host name into packet */

  do {
    if ((s = strchr(name, '.')) == NULL) {
      s = name + name_len;
    }

    n = s - name;               /* Chunk length */
    *p++ = n;                   /* Copy length */
    for (i = 0; i < n; i++) {   /* Copy chunk */
      *p++ = name[i];
    }

    if (*s == '.') {
      n++;
    }

    name += n;
    name_len -= n;

  } while (*s != '\0');

  *p++ = 0;                     /* Mark end of host name */
  *p++ = 0;                     /* Well, lets put this byte as well */
  *p++ = (unsigned char) qtype; /* Query Type */

  *p++ = 0;
  *p++ = 1;                     /* Class: inet, 0x0001 */

  assert (p < pkt + sizeof(pkt));
  n = p - pkt;                  /* Total packet length */

  {
    char *resolver = inet_ntoa(dns->sa.sin_addr);
    freelog(LOG_DEBUG, "dq sending dns packet n=%d to resolver=%s", n,
            resolver);
  }
  
  nb = sendto(dns->sock, pkt, n, 0, (struct sockaddr *) &dns->sa,
              sizeof(dns->sa));

  if (nb != n) {
    if (nb == -1) {
      freelog(LOG_ERROR, _("Failed to send DNS query: sendto: %s."),
              mystrsocketerror(mysocketerrno()));
    } else {
      freelog(LOG_ERROR, _("Function sendto failed to send entire DNS query."));
    }
    call_user(dns, query);
    destroy_query(dns, query);
    return;
  }

  hash_insert(dns->active, UINT16_T_TO_PTR(query->tid), query);
  query->refcount++;
}
/**************************************************************************
  ...
**************************************************************************/
void dns_check_expired(struct dns *dns)
{
  time_t now;

  now = time(NULL);
  freelog(LOG_DEBUG, "dce dns_check_expired dns=%p now=%lu", dns, now);
  
  /* Cleanup expired active queries */
  hash_iterate(dns->active, void *, key, struct query *, query) {
    if (query->expire < now) {
      freelog(LOG_DEBUG, "dce active query timeout tid=%x expire=%lu",
              query->tid, query->expire);
      query->addrlen = 0;
      call_user(dns, query);
      destroy_query(dns, query);
    }
  } hash_iterate_end;
  
  /* Cleanup cached queries */
  hash_iterate(dns->cache, char *, name, struct query *, query) {
    if (query->expire < now) {
      freelog(LOG_DEBUG, "dce query expired in cache tid=%x expire=%lu "
              "(name \"%s\")", query->tid, query->expire, name);
      destroy_query(dns, query);
    }
  } hash_iterate_end;
}
