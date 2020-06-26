#ifndef __OPWEB_H__
#define __OPWEB_H__
#include "event.h"
#include "interface/log.h"
#include "libubox/list.h"
#include "openssl/types.h"
#include "openssl/ssl.h"
#include "interface/http.h"
#include "ophttp.h"
#include "interface/public.h"
#include "interface/bus.h"

#define HTTPS_CLIENT_READ_BUF 8096
#define MAX_OPWEB_THREAT_NUM 5
#define MAX_OPWEB_CLIENT_NUM 10
#define MAX_OPWEB_HTTPS_CLIENT_NUM 100

struct ssl_client{
	struct list_head node;
	SSL *ssl;
	X509 *cert;
	unsigned int client_id;
	int enable;
	int client_fd;
	struct sockaddr_in addr;
	struct opweb_thread *thread;
	struct event *ev_read;
	unsigned char read_buf[HTTPS_CLIENT_READ_BUF];
	struct http_proto proto;
	struct timeval t;
	struct event *alive_timer;
};

struct opweb *opweb_self;

#define opweb_log_error(fmt...) log_err(opweb_self->log,fmt)
#define opweb_log_warn(fmt...) log_warn(opweb_self->log,fmt)
#define opweb_log_info(fmt...) log_info(opweb_self->log,fmt)
#define opweb_log_debug(fmt...) log_debug(opweb_self->log,fmt)
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

struct http_conf {
	unsigned int port;
};

struct https_conf {
	unsigned int port;
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
	struct client_idle idle_https;
	struct client_busy busy_https;
	pthread_rwlock_t rwlock;
	SSL_CTX *openssl_ctx;
	char pub_path[OPSERVER_PATH_SIZE];
	char priv_path[OPSERVER_PATH_SIZE];
	char ca_trust_path[OPSERVER_PATH_SIZE];
	char www_root[OPSERVER_PATH_SIZE];
	struct http_conf hp_conf;
	struct http_conf hps_conf;
	
	struct ssl_client client_https[MAX_OPWEB_HTTPS_CLIENT_NUM];
	unsigned char bus_webbuf[BUS_RCV_BUF_MAX];
};

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
