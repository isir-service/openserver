#ifndef __OPHTTPS_H__
#define __OPHTTPS_H__
#include "event.h"
#include "openssl/ssl.h"
#include "openssl/types.h"
#include "libubox/list.h"
#include "opweb_pub.h"

struct ssl_client *opweb_alloc_idle_ssl_client(void);

void opweb_release_idle_ssl_client(struct ssl_client *client);

void opweb_https_client_free(struct ssl_client *client);

void opweb_https_accept(evutil_socket_t fd, short what, void *arg);

void opweb_https_read(evutil_socket_t fd, short what, void *arg);

void opweb_https_timer(int s, short what, void *arg);

int opweb_cb_fastcgi(struct ssl_client *client, unsigned char *write_buf, unsigned int write_size);

#endif
