#include "opweb.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

struct opweb *self = NULL;

struct opweb *get_h_opweb(void)
{
	assert(self);

	return self;
}


struct opweb *opweb_init(void)
{
	struct opweb *web = NULL;
	int i=0;

	web = calloc(1, sizeof(struct opweb));
	if (!web)
		goto out;

	pthread_rwlock_init(&web->rwlock, NULL);

	INIT_LIST_HEAD(&web->idle.head);
	INIT_LIST_HEAD(&web->busy.head);

	for (i = 0; i < MAX_OPWEB_CLIENT_NUM; i ++) {
		INIT_LIST_HEAD(&web->client[i].node);
		web->client[i].client_id = i;
		list_add_tail(&web->client[i].node, &web->idle.head);
		web->idle.num++;
	}
	
	self = web;

	return web;

out:

	opweb_exit(web);
	return NULL;
}

void opweb_exit(struct opweb *web)
{
	(void)web;

	return;
}

void opweb_bus_cb(void *h, unsigned int from_module, unsigned int from_sub_id,unsigned int to_sub_id, void *data, unsigned int size, void *arg)
{
	(void)h;
	(void)from_module;
	(void)from_sub_id;
	(void)to_sub_id;
	(void)data;
	(void)size;
	(void)arg;

	return;

}

void opweb_bus_disconnect(void *h,void *arg)
{
	(void)h;
	(void)arg;

	return;
}

static struct opweb_thread *opweb_get_thread(void)
{
	struct opweb *web = get_h_opweb();
	unsigned int num_client = web->thread[0].client_num;
	int index = 0;
	int i = 0;
	
	pthread_rwlock_rdlock(&web->rwlock);
	for (i = 1; i < MAX_OPWEB_THREAT_NUM; i++) {
		if (web->thread[i].client_num < num_client) {
			num_client = web->thread[i].client_num;
			index = i;
		}
	}

	pthread_rwlock_unlock(&web->rwlock);

	return &(web->thread[index]);

}

struct opweb_client *opweb_alloc_idle_client(void)
{
	struct opweb *web = get_h_opweb();
	struct opweb_client *client = NULL;
	pthread_rwlock_wrlock(&web->rwlock);

	if (list_empty(&web->idle.head)) {
		goto out;
	}

	client = list_first_entry(&web->idle.head, struct opweb_client, node);
	if (!client) {
		goto out;
	}
	
	list_move_tail(&client->node, &web->busy.head);
	web->idle.num--;
	web->busy.num++;
	client->enable = 1;
	pthread_rwlock_unlock(&web->rwlock);
	return client;

out:
	
	pthread_rwlock_unlock(&web->rwlock);
	return NULL;
}

void opweb_free_busy_client(struct opweb_client *client)
{
	struct opweb *web = get_h_opweb();
	if (!client)
		return;

	list_move_tail(&client->node, &web->idle.head);
	web->busy.num--;
	web->idle.num++;
	client->enable = 0;
	return;
}

void release_client(struct opweb_client *client)
{

	struct opweb *web = get_h_opweb();

	if (!client)
		return;

	pthread_rwlock_wrlock(&web->rwlock);
	if (!client->enable) {
		pthread_rwlock_unlock(&web->rwlock);
		return;
	}
	opweb_free_busy_client(client);
	bufferevent_free(client->buffer);
	close(client->fd);
	evtimer_del(client->alive_timer);
	client->thread->client_num--;
	pthread_rwlock_unlock(&web->rwlock);
	opweb_log_debug("thread [%x]release situation:busy[%u], idle[%u]\n", (unsigned int)client->thread->thread_id, web->busy.num, web->idle.num);
	return;
}


void opweb_client_read(struct bufferevent *bev, void *ctx)
{
	(void)bev;
	(void)ctx;
	char data[1024] = {};
	struct opweb_client *client = (struct opweb_client *)ctx;
	bufferevent_read(bev, data, 1024);
	printf("recv[%d]:%s\n", client->fd,data);
	return;
}

void opweb_client_event(struct bufferevent *bev, short what, void *ctx)
{
	(void)bev;
	(void)what;
	(void)ctx;
	
	struct opweb_client *client = NULL;
	
	if (what & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) {
		client = (struct opweb_client *)ctx;		
		if (!client)
			return;

		release_client(client);
	}
	
	return;
}

void opweb_http_accept(evutil_socket_t fd, short what, void *arg)
{
	(void)fd;
	(void)what;
	(void)arg;

	int client_fd = 0;
	struct opweb_thread *thread = NULL;
	struct opweb_client *client = NULL;
	socklen_t len = 0;
	struct sockaddr_in addr;
	struct opweb *web = get_h_opweb();
	memset(&addr, 0, sizeof(addr));
	
	client_fd = accept(fd, (struct sockaddr*)&addr, &len);
	if (client_fd < 0) {
		opweb_log_warn("accept failed:%s\n", strerror(errno));
		goto out;
	}

	
	thread = opweb_get_thread();
	if (!thread) {
		opweb_log_warn("get opweb thread failed\n");
		goto out;
	}
	
	opweb_log_debug("thread [%u][%x]\n", thread->index, (unsigned int)thread->thread_id);
	opweb_log_debug("accept situation:busy[%u], idle[%u]\n", web->busy.num, web->idle.num);
	client = opweb_alloc_idle_client();
	if (!client) {
		opweb_log_warn("alloc idle client failed\n");
		goto out;
	}

	pthread_rwlock_wrlock(&web->rwlock);

	client->thread = thread;
	client->buffer = bufferevent_socket_new(thread->base, client_fd, 0);
	if (!client->buffer) {
		opweb_log_warn("client [%u] create bufferevent socket failed\n", client->client_id);
		opweb_free_busy_client(client);
		pthread_rwlock_unlock(&web->rwlock);
		goto out;
	}

	client->alive_timer = evtimer_new(thread->base, opweb_timer, client);
	if (!client->alive_timer) {
		opweb_log_warn("client [%u] alloc evtimer_new failed\n", client->client_id);
		opweb_free_busy_client(client);
		pthread_rwlock_unlock(&web->rwlock);
		goto out;
	}

	client->t.tv_sec = 10;
	if (event_add(client->alive_timer, &client->t) < 0) {
		
		opweb_log_warn("client [%u] add evtimer_new failed\n", client->client_id);
		opweb_free_busy_client(client);
		pthread_rwlock_unlock(&web->rwlock);
		goto out;

	}

	bufferevent_setcb(client->buffer, opweb_client_read, NULL, opweb_client_event, client);
	bufferevent_enable(client->buffer, EV_READ);
	memcpy(&client->addr, &addr, sizeof(client->addr));
	client->fd = client_fd;
	thread->client_num++;
	pthread_rwlock_unlock(&web->rwlock);
	opweb_log_debug("accept success:client_id [%u],thread_index [%u], thread_id[%x], thread_serv_client_num[%u]\n",
			client->client_id, thread->index, (unsigned int)thread->thread_id, thread->client_num);
	return;
out:
	if (client_fd)
		close(client_fd);
	return;
}


void *opweb_thread_job(void *arg)
{
	if (!arg)
		goto out;
	
	event_base_loop(arg,EVLOOP_NO_EXIT_ON_EMPTY);
	opweb_log_debug("thread exit\n");
out:
	pthread_join(pthread_self() , NULL);
	return NULL;
}

void opweb_timer(int s, short what, void *arg)
{
	(void)s;
	(void)what;
	(void)arg;

	struct opweb_client *client = (struct opweb_client *)arg;
	release_client(client);
	return;
}


