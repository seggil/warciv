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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
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
#ifdef HAVE_WINSOCK
#include <winsock.h>
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


#ifdef WIN32_NATIVE
typedef int socklen_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#endif /* WIN32_NATIVE */


#define  DNS_MAX      1025      /* Maximum host name */
#define  DNS_PACKET_LEN    2048 /* Buffer size for DNS packet */
#define  MAX_CACHE_ENTRIES  10000 /* Dont cache more than that */

struct dns;
/*
 * User query. Holds mapping from application-level ID to DNS transaction id,
 * and user defined callback function.
 */
struct query {
  time_t expire;                /* Time when this query expire */
  uint16_t tid;                 /* UDP DNS transaction ID, big endian */
  uint16_t qtype;               /* Query type */
  char name[DNS_MAX];           /* Host name */
  void *ctx;                    /* Application context */
  dns_callback_t callback;      /* User callback routine */
  unsigned char addr[DNS_MAX];  /* Host address */
  size_t addrlen;               /* Address length */
  struct dns *dns;
};

/* Resolver descriptor. */
struct dns {
  int sock;                     /* UDP socket used for queries */
  struct sockaddr_in sa;        /* DNS server socket address */
  uint16_t tid;                 /* Latest tid used */

  struct hash_table *active;
  struct hash_table *cache;

  char buf[4096];
  int buflen;
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
  return (dns->sock);
}

/**************************************************************************
  Fetch name from DNS packet
**************************************************************************/
static void fetch(const uint8_t * pkt, int pktsiz, const uint8_t * s,
		  char *dst, int dstlen)
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
  int ret = 0;

#ifdef WIN32_NATIVE
  int i;
  LONG err;
  HKEY hKey, hSub;
  char subkey[512], value[128], *key =
    "SYSTEM\\ControlSet001\\Services\\Tcpip\\Parameters\\Interfaces";

  if ((err = RegOpenKey(HKEY_LOCAL_MACHINE, key, &hKey)) != ERROR_SUCCESS) {
    freelog(LOG_ERROR, "Cannot open registry key %s: %d", key, err);
    ret--;
  } else {
    for (ret--, i = 0; RegEnumKey(hKey, i, subkey,
				  sizeof(subkey)) == ERROR_SUCCESS; i++) {
      DWORD type, len = sizeof(value);
      if (RegOpenKey(hKey, subkey, &hSub) == ERROR_SUCCESS &&
          (RegQueryValueEx(hSub, "DhcpNameServer", 0,
			   &type, value, &len) == ERROR_SUCCESS ||
           RegQueryValueEx(hSub, "DhcpNameServer", 0,
			   &type, value, &len) == ERROR_SUCCESS)) {
        dns->sa.sin_addr.s_addr = inet_addr(value);
        ret++;
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
    for (ret--; fgets(line, sizeof(line), fp) != NULL;) {
      if (sscanf(line, "nameserver %d.%d.%d.%d", &a, &b, &c, &d) == 4) {
        dns->sa.sin_addr.s_addr = htonl(a << 24 | b << 16 | c << 8 | d);
        ret++;
        break;
      }
    }
    fclose(fp);
  }
#endif /* WIN32_NATIVE */

  return (ret);
}

/**************************************************************************
  ...
**************************************************************************/
struct dns *dns_init(void)
{
  struct dns *dns;
  int rcvbufsiz = 64 * 1024;
  uint32_t seed;

  dns = fc_malloc(sizeof(struct dns));
  memset(dns, 0, sizeof(struct dns));
  dns->sock = -1;
  
  if ((dns->sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    freelog(LOG_ERROR, _("Failed to create UDP socket: %s"), mystrsocketerror());
    goto FAILED;
  }
  
  if (my_nonblock(dns->sock) == -1) {
    freelog(LOG_ERROR, _("Failed to set non-blocking mode on UDP socket: %s"),
	    mystrsocketerror());
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
			    " setsockopt: %s"), mystrsocketerror());
      goto FAILED;
    }
  
  dns->active = hash_new(hash_fval_uint16_t, hash_fcmp_uint16_t);
  dns->cache = hash_new(hash_fval_string, hash_fcmp_string_ci);

  /* Not exactly cryptographically kosher,
     but good enough I guess :) */
  seed = (uint32_t) time(NULL);
  dns->tid = hash_fval_uint32_t(UINT32_T_TO_PTR(seed), 0xffffffff);
  
  return dns;
  
 FAILED:
  dns_fini(dns);  
  return NULL;
}

/**************************************************************************
  ...
**************************************************************************/
static void destroy_query(struct query *query)
{
  struct dns *dns;
  freelog(LOG_DEBUG, "deq destroy_query query=%p", query); /*ASYNCDEBUG*/

  assert(query != NULL);
  assert(query->dns != NULL);
  dns = query->dns;
  freelog(LOG_DEBUG, "deq   removing tid=%d from active table", query->tid); /*ASYNCDEBUG*/
  hash_delete_entry(dns->active, UINT16_T_TO_PTR(query->tid));
  freelog(LOG_DEBUG, "deq   removing name=\"%s\" from cache", query->name); /*ASYNCDEBUG*/
  hash_delete_entry(dns->cache, query->name);
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
  freelog(LOG_DEBUG, "fcq find_cached_query \"%s\"", name); /*ASYNCDEBUG*/

  freelog(LOG_DEBUG, "fcq   cache contains:"); /*ASYNCDEBUG*/
  freelog(LOG_DEBUG, "fcq     entries=%d", hash_num_entries(dns->cache)); /*ASYNCDEBUG*/
  freelog(LOG_DEBUG, "fcq     buckets=%d", hash_num_buckets(dns->cache)); /*ASYNCDEBUG*/
  freelog(LOG_DEBUG, "fcq     deleted=%d", hash_num_deleted(dns->cache)); /*ASYNCDEBUG*/
  hash_iterate(dns->cache, char *, name, struct query *, pq) { /*ASYNCDEBUG*/
    freelog(LOG_DEBUG, "fcq   \"%s\" --> query %p", name, pq); /*ASYNCDEBUG*/
  } hash_iterate_end; /*ASYNCDEBUG*/
  
  q = hash_lookup_data(dns->cache, name);
  freelog(LOG_DEBUG, "fcq   %sfound", q ? "" : "NOT "); /*ASYNCDEBUG*/
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
void dns_cancel(struct dns *dns, const void *context)
{
  freelog(LOG_DEBUG, "dns_cancel dns=%p context=%p", dns, context); /*ASYNCDEBUG*/
  hash_iterate(dns->active, void *, key, struct query *, query) {
    if (query->ctx == context) {
      destroy_query(query);
    }
  } hash_iterate_end;
}

/**************************************************************************
  ...
**************************************************************************/
static void call_user(struct dns *dns, struct query *query)
{
  freelog(LOG_DEBUG, "cu call_user query tid=%d name=%s", /*ASYNCDEBUG*/
	  query->tid, query->name); /*ASYNCDEBUG*/
  assert(query->callback != NULL);
  query->callback(query->ctx,(enum dns_query_type) query->qtype,
		  query->name, query->addr, query->addrlen);
  freelog(LOG_DEBUG, "cu   returned from callback"); /*ASYNCDEBUG*/
  query->callback = NULL;
  query->ctx = NULL;
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
  bool found = FALSE, stop = FALSE;

  freelog(LOG_DEBUG, "pu parse_udp"); /*ASYNCDEBUG*/
  
  /* We sent 1 query. We want to see more that 1 answer. */
  header = (struct header *) dns->buf;
  if (ntohs(header->nqueries) != 1) {
    freelog(LOG_DEBUG, "pu   nqueries is not 1 (%d) :(", header->nqueries); /*ASYNCDEBUG*/
    return;
  }

  /* Return if we did not send that query */
  if ((q = find_active_query(dns, header->tid)) == NULL) {
    freelog(LOG_DEBUG, "pu   unknown tid=%d :(", header->tid); /*ASYNCDEBUG*/
    return;
  }

  /* Check respone code */
  flags = ntohs(header->flags);
  freelog(LOG_DEBUG, "pu   header->flags=0x%04x flags=0x%04x", header->flags, flags); /*ASYNCDEBUG*/
  rcode = flags & 0x000f;
  freelog(LOG_DEBUG, "pu   rcode=%d", rcode); /*ASYNCDEBUG*/
  
  if (rcode == 1) {
    freelog(LOG_ERROR,
	    _("DNS query failed: format error (%d)."), rcode);
    return;
  }

  if (rcode == 2) {
    freelog(LOG_VERBOSE,
	    _("DNS query failed: server failure (%d)."), rcode);
    return;
  }

  if (rcode == 3) {
    freelog(LOG_VERBOSE,
	    _("DNS query failed: name error - no such domain (%d)."),
	    rcode);
    return;
  }

  if (rcode != 0) {
    freelog(LOG_VERBOSE, _("Unknown DNS query failure (%d)."), rcode);
    return;
  }

  /* Received 0 answers */
  if (header->nanswers == 0) {
    freelog(LOG_DEBUG, "pu   got 0 answers :("); /*ASYNCDEBUG*/
    q->addrlen = 0;
    call_user(dns, q);
    return;
  }

  /* Skip host name */
  e = (const unsigned char *)(dns->buf + dns->buflen);
  p = &header->data[0];
  for (nlen = 0; p < e && *p != '\0'; p++)
    nlen++;

  qtype = ntohs(*(uint16_t *) (p + 1));

  /* We sent query class 1, query type 1 */
  if (p + 5 > e || qtype != q->qtype) {
    freelog(LOG_DEBUG, "pu   returning on class/query mismatch: " /*ASYNCDEBUG*/
	    "p+5=%p e=%p qtype=%d q->qtype=%d", /*ASYNCDEBUG*/
	    p+5, e, qtype, q->qtype); /*ASYNCDEBUG*/
    return;
  }

  /* Go to the first answer section */
  p += 5;

  /* Loop through the answers, we want A type answer */
  for (found = stop = 0; !stop && p + 12 < e;) {

    /* Skip possible name in CNAME answer */
    if (*p != 0xc0) {
      while (*p && p + 12 < e)
        p++;
      p--;
    }

    /* XXX check: is this supposed to be ntohs, or htons?
       (was htons in original tadns sources (similarly below) */
    type = ntohs(((uint16_t *) p)[1]);

    freelog(LOG_DEBUG, "pu   got answer type=%d", type); /*ASYNCDEBUG*/
    if (type == 5) {
      /* CNAME answer. shift to the next section */
      dlen = ntohs(((uint16_t *) p)[5]);
      p += 12 + dlen;
    } else if (type == q->qtype) {
      found = stop = 1;
    } else {
      stop = 1;
    }
  }

  if (found && p + 12 < e) {
    dlen = htons(((uint16_t *) p)[5]); /* XXX ntohs? */
    p += 12;

    if (p + dlen <= e) {
      /* Add to the cache */
      memcpy(&ttl, p - 6, sizeof(ttl));
      q->expire = time(NULL) + (time_t) ntohl(ttl);
      freelog(LOG_DEBUG, "pu   got expire=%ld ttl=%ld", 
	      q->expire, ntohl(ttl)); /*ASYNCDEBUG*/
      
      /* Call user */
      if (q->qtype == DNS_MX_RECORD) {
        /* Skip 2 byte preference field */
        fetch((const unsigned char *) (dns->buf), 
	      dns->buflen, p + 2, name, sizeof(name));
        p = (const unsigned char *) name;
        dlen = strlen(name) + 1;
      } else if (q->qtype == DNS_PTR_RECORD) {
        freelog(LOG_DEBUG, "pu   extracting DNS_PTR_RECORD"); /*ASYNCDEBUG*/
        fetch((const unsigned char *) (dns->buf),
	      dns->buflen, p, name, sizeof(name));
        p = (const unsigned char *) name;
        dlen = strlen(name) + 1;
        freelog(LOG_DEBUG, "pu   p=\"%s\" dlen=\"%d\"", p, dlen); /*ASYNCDEBUG*/
      }

      q->addrlen = dlen;
      if (q->addrlen > sizeof(q->addr))
        q->addrlen = sizeof(q->addr);
      memcpy(q->addr, p, q->addrlen);
      call_user(dns, q);

      /* Move query to cache */
      freelog(LOG_DEBUG, "pu   caching \"%s\" with query=%p", q->name, q); /*ASYNCDEBUG*/
      if (!hash_insert(dns->cache, q->name, q)) {
        freelog(LOG_DEBUG, _("Failed to insert DNS result into cache!")); /*ASYNCDEBUG*/
      }
    }
  }
}

/**************************************************************************
  Check for packets on dns udp socket.
**************************************************************************/
void dns_poll(struct dns *dns)
{
  struct sockaddr_in sa;
  socklen_t len = sizeof(sa);
  int nb;

  freelog(LOG_DEBUG, "dp dns_poll"); /*ASYNCDEBUG*/

  /* Check our socket for new stuff */
  nb = recvfrom(dns->sock, dns->buf + dns->buflen,
		sizeof(dns->buf) - dns->buflen, 0,
                (struct sockaddr *) &sa, &len);
  freelog(LOG_DEBUG, "dp   recvfrom got nb=%d, from %s", /*ASYNCDEBUG*/
	  nb, inet_ntoa(sa.sin_addr)); /*ASYNCDEBUG*/

  if (memcmp(&sa.sin_addr, &dns->sa.sin_addr, sizeof(sa.sin_addr))) {
    freelog(LOG_VERBOSE,
	    _("Ignoring UDP packet from %s (not from DNS resolver %s)."),
	    inet_ntoa(sa.sin_addr), inet_ntoa(dns->sa.sin_addr));
    return;
  }
  
  if (nb < 0) {
    if (my_socket_would_block() || my_socket_operation_in_progess()) {
      freelog(LOG_DEBUG, "dp   recvfrom would block"); /*ASYNCDEBUG*/
    } else {
      freelog(LOG_ERROR, _("Failed reading DNS response: recvfrom: %s"),
	      mystrsocketerror());
    }
  }
  
  if (nb > 0) {
    dns->buflen += nb;
    if (dns->buflen > sizeof(struct header)) {
      parse_udp(dns);
    } else {
      freelog(LOG_VERBOSE, _("Ignoring UDP packet since it is smaller "
			     "than the size of a DNS packet header (%d < %d)."),
	      dns->buflen, sizeof(struct header));
    }
  }

  dns->buflen = 0;
  
}

/**************************************************************************
  Cleanup
**************************************************************************/
void dns_fini(struct dns *dns)
{
  if (dns->sock != -1)
    (void) my_closesocket(dns->sock);

  if (dns->active) {
    hash_iterate(dns->active, void *, key, struct query *, query) {
      destroy_query(query);
    } hash_iterate_end;
    hash_free(dns->active);
  }

  if (dns->cache) {
    hash_iterate(dns->cache, char *, name, struct query *, query) {
      destroy_query(query);
    } hash_iterate_end;
    hash_free(dns->cache);
  }

  free(dns);
}

/**************************************************************************
  Queue the resolution
**************************************************************************/
void
dns_queue(struct dns *dns, void *ctx, const char *name,
	  enum dns_query_type qtype, dns_callback_t callback)
{
  struct query *query;
  struct header *header;
  int i, n, name_len, nb;
  char pkt[DNS_PACKET_LEN], *p;
  const char *s;
  time_t now = time(NULL);
  unsigned int hv;

  freelog(LOG_DEBUG, "dq dns_queue dns=%p ctx=%p name=\"%s\"" /*ASYNCDEBUG*/
	  " qtype=%d callback=%p", dns, ctx, name, qtype, callback); /*ASYNCDEBUG*/
  
  /* Search the cache first */
  if ((query = find_cached_query(dns, qtype, name)) != NULL) {
    freelog(LOG_DEBUG, "dq   found cached query for \"%s\": query %p", name, query); /*ASYNCDEBUG*/
    query->callback = callback;
    query->ctx = ctx;
    call_user(dns, query);
    if (query->expire < now) {
      destroy_query(query);
    }
    return;
  }

  /* Allocate new query */
  query = fc_malloc(sizeof(struct query));

  /* Init query structure */
  query->ctx = ctx;
  query->qtype = (uint16_t) qtype;
  hv = hash_fval_uint16_t(UINT16_T_TO_PTR (dns->tid), 0xffff);
  query->tid = (uint16_t) hv;
  dns->tid = query->tid;
  query->callback = callback;
  query->expire = now + DNS_QUERY_TIMEOUT;
  query->dns = dns;

  /* copy over name, converted to lower case */
  for (p = query->name; *name &&
	 p < query->name + sizeof(query->name) - 1; name++, p++)
    *p = tolower(*name);
  *p = '\0';
  name = query->name;

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
    if ((s = strchr(name, '.')) == NULL)
      s = name + name_len;

    n = s - name;               /* Chunk length */
    *p++ = n;                   /* Copy length */
    for (i = 0; i < n; i++)     /* Copy chunk */
      *p++ = name[i];

    if (*s == '.')
      n++;

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
    freelog(LOG_DEBUG, "dq   sending dns packet n=%d to resolver=%s", n, /*ASYNCDEBUG*/
            resolver); /*ASYNCDEBUG*/
  }
  
  nb = sendto(dns->sock, pkt, n, 0,
              (struct sockaddr *) &dns->sa,
	      sizeof(dns->sa));

  if (nb != n) {
    if (nb == -1) {
      freelog(LOG_ERROR, _("Failed to send DNS query: sendto: %s."),
	      mystrsocketerror());
    } else {
      freelog(LOG_ERROR, _("Function sendto failed to send entire DNS query."));
    }
    callback(ctx, qtype, name, NULL, 0);
    destroy_query(query);
    return;
  }

  if (!hash_insert(dns->active, UINT16_T_TO_PTR(query->tid), query)) {
    freelog(LOG_ERROR, _("Failed to insert into DNS active query table!"));
  }
}
/**************************************************************************
  ...
**************************************************************************/
void dns_check_expired(struct dns *dns)
{
  time_t now;

  freelog(LOG_DEBUG, "dce dns_check_expired dns=%p", dns); /*ASYNCDEBUG*/
  
  now = time(NULL);
  
  /* Cleanup expired active queries */
  hash_iterate(dns->active, void *, key, struct query *, query) {
    if (query->expire < now) {
      freelog(LOG_DEBUG, "dce   active query expired tid=%d", 
	      query->tid); /*ASYNCDEBUG*/
      freelog(LOG_DEBUG, "dcz   now=%ld expire=%ld", 
	      now, query->expire); /*ASYNCDEBUG*/
      query->addrlen = 0;
      if (query->callback) {
        call_user(dns, query);
      }
      destroy_query(query);
    }
  } hash_iterate_end;
  
  /* Cleanup cached queries */
  hash_iterate(dns->cache, char *, name, struct query *, query) {
    if (query->expire < now) {
      freelog(LOG_DEBUG, "dce   query expired in cache tid=%d", query->tid); /*ASYNCDEBUG*/
      destroy_query(query);
    }
  } hash_iterate_end;
}
