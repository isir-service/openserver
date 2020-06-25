#ifndef __OPHTTPS_H__
#define __OPHTTPS_H__
#include "event.h"
#include "openssl/ssl.h"
#include "openssl/types.h"
#include "libubox/list.h"
#include "opweb.h"

struct ssl_client *opweb_alloc_idle_ssl_client(void);

void opweb_release_idle_ssl_client(struct ssl_client *client);


void opweb_https_accept(evutil_socket_t fd, short what, void *arg);

void opweb_https_read(evutil_socket_t fd, short what, void *arg);




#endif
