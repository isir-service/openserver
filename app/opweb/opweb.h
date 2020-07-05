#ifndef __OPWEB_H__
#define __OPWEB_H__
#include "event.h"
#include "interface/log.h"
#include "libubox/list.h"
#include "interface/http.h"
#include "interface/public.h"
#include "interface/bus.h"
#include "opweb_pub.h"

struct opweb *get_h_opweb(void);

struct opweb *opweb_init(void);

void opweb_exit(struct opweb *web);

void opweb_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id,unsigned int to_sub_id, void *data, unsigned int size, void *arg);

void opweb_bus_disconnect(void *h,void *arg);
void opweb_http_accept(evutil_socket_t fd, short what, void *arg);
void *opweb_thread_job(void *arg);

void opweb_free_busy_client(struct opweb_client *client);
struct opweb_client *opweb_alloc_idle_client(void);

void release_client(struct opweb_client *client);

void opweb_client_read(struct bufferevent *bev, void *ctx);
void opweb_client_event(struct bufferevent *bev, short what, void *ctx);

void opweb_timer(int s, short what, void *arg);
void opweb_bufferevent_flush(struct bufferevent *bev);
struct opweb_thread *opweb_get_thread(void);

int opweb_config_parse(char *conf_path);

#endif
