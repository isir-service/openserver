#ifndef __OP_BUS_H__
#define __OP_BUS_H__
#include "event.h"
#include "event2/bufferevent.h"
#include "interface/module.h"
#include "pthread.h"

#define MAX_JOB_THREAT_NUM 5
#define CLIENT_RECV_BUF_SIZE 40960 /*40k*/

struct opbus_header {
	unsigned char pad1;
	unsigned char pad2;
	unsigned char type;
	unsigned int from_module;
	unsigned int from_sub_id;
	unsigned int to_module;
	unsigned int to_sub_id;
	unsigned int payload_size;
}__attribute__((packed));


struct client_info {
	char module_name[MODULE_NAME_SIZE];
	struct sockaddr_in addr;
	int client_fd;
	struct bufferevent *buffer;
	struct evbuffer *evbuff;
	size_t water_level;
	int enable;
	unsigned char recv_buf[CLIENT_RECV_BUF_SIZE];
	unsigned int read_index;
	struct opbus_header header;
	struct op_thread *thread;
	int recv_type;
};

struct op_thread {
	pthread_t thread_id;
	unsigned int client_num;
	struct event_base *ebase_job;
	struct event *trigger;
};

struct _op_bus {
	void *log;
	struct event_base *ebase_sche;
	struct event *sch_read;
	int fd;
	unsigned int client_num;
	struct op_thread thread[MAX_JOB_THREAT_NUM];
	struct client_info client[MODULE_MAX];
	pthread_rwlock_t rwlock;
};

struct _op_bus * opbus_init(void);

void opbus_exit(struct _op_bus *bus);

void opbus_accept(evutil_socket_t fd, short what, void *arg);

void *opbus_thread_job(void *arg);
void opbus_trigger(int s, short what, void *arg);


#endif
