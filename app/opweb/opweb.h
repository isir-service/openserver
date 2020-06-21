#ifndef __OPWEB_H__
#define __OPWEB_H__
#include "event.h"
#include "interface/log.h"
#include "libubox/list.h"

#define MAX_OPWEB_THREAT_NUM 5
#define MAX_OPWEB_CLIENT_NUM 10

struct opweb *self;

#define opweb_log_error(fmt...) log_err(self->log,fmt)
#define opweb_log_warn(fmt...) log_warn(self->log,fmt)
#define opweb_log_info(fmt...) log_info(self->log,fmt)
#define opweb_log_debug(fmt...) log_debug(self->log,fmt)
struct opweb_thread;

struct opweb_client {

	struct list_head node;
	struct sockaddr_in addr;
	int fd;
	struct event *alive_timer;
	struct timeval t;
	struct bufferevent *buffer;
	int enable;
	unsigned int client_id;
	struct opweb_thread *thread;
};

struct opweb_thread {
	pthread_t thread_id;
	struct event_base *base;
	unsigned int client_num;
	unsigned int index;
};

struct client_idle {
	unsigned int num;
	struct list_head head;
};

struct client_busy {
	unsigned int num;
	struct list_head head;
};

struct opweb {
	struct event_base *base;
	void *bus;
	void *log;
	int http_fd;
	int https_fd;
	struct event *ehttp;
	struct event *ehttps;
	struct opweb_thread thread[MAX_OPWEB_THREAT_NUM];
	struct opweb_client client[MAX_OPWEB_CLIENT_NUM];
	struct client_idle idle;
	struct client_busy busy;
	pthread_rwlock_t rwlock;
};

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

#endif
