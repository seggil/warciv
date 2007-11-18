/*
 * Copyright (c) 2004-2005 Sergey Lyubka <valenok@gmail.com>
 *
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Sergey Lyubka wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 */

/*
 * Simple asynchronous DNS resolver.
 * Can resolve A records (IP addresses for a given name),
 * MX records (IP addresses of mail exchanges for a given domain),
 * and now PTR records.
 * It holds resolved IP addresses in a cache.
 */

#ifndef DNS_HEADER_INCLUDED
#define DNS_HEADER_INCLUDED

enum dns_query_type {
  DNS_A_RECORD   = 0x01, 
  DNS_PTR_RECORD = 0x0c, /* i.e. 12 */
  DNS_MX_RECORD  = 0x0f
};

/*
 * User defined function that will be called when DNS reply arrives for
 * requested hostname. The addrlen == 0 indicates lookup failure or timeout.
 */
typedef void (*dns_callback_t)(void *context, enum dns_query_type qtype,
		const char *name, const unsigned char *addr, size_t addrlen);

#define	DNS_QUERY_TIMEOUT	30	/* Query timeout, seconds */

struct dns;
extern struct dns *dns_init(void);
extern void	dns_fini(struct dns *);
extern int	dns_get_fd(struct dns *);
extern void	dns_queue(struct dns *, void *context, const char *hostname,
			enum dns_query_type type, dns_callback_t callback);
extern void	dns_cancel(struct dns *, const void *context);
extern void	dns_poll(struct dns *);
extern void     dns_check_expired (struct dns *);

#endif /* DNS_HEADER_INCLUDED */
