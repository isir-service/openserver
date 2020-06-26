#include "opweb.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include "libxml/tree.h"
#include "libubox/utils.h"
#include "libxml/parser.h"


struct opweb *opweb_self = NULL;

struct opweb *get_h_opweb(void)
{
	assert(opweb_self);

	return opweb_self;
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
	INIT_LIST_HEAD(&web->idle_https.head);
	INIT_LIST_HEAD(&web->busy_https.head);


	for (i = 0; i < MAX_OPWEB_CLIENT_NUM; i ++) {
		INIT_LIST_HEAD(&web->client[i].node);
		web->client[i].client_id = i;
		list_add_tail(&web->client[i].node, &web->idle.head);
		web->idle.num++;
	}

	for (i = 0; i < MAX_OPWEB_HTTPS_CLIENT_NUM; i ++) {
		INIT_LIST_HEAD(&web->client_https[i].node);
		web->client_https[i].client_id = i;
		list_add_tail(&web->client_https[i].node, &web->idle_https.head);
		web->idle_https.num++;
	}
	
	opweb_self = web;

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

struct opweb_thread *opweb_get_thread(void)
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

	char *str = "HTTP/1.1 307 Temporary Redirect\r\n"
	"Server: opweb/1.1\r\n"
	"Non-Authoritative-Reason: HSTS\r\n"
	"Location: https://192.168.1.15:60001\r\n\r\n";
	int fd  =0;
	struct opweb_client *client = NULL;
	client = (struct opweb_client *)ctx;
	if (!client)
		return;

	fd = bufferevent_getfd(bev);
	write(fd, str, strlen(str));
	event_add(client->alive_timer, &client->t);
	release_client(client);
	return;
}

void opweb_client_event(struct bufferevent *bev, short what, void *ctx)
{
	(void)bev;
	(void)what;
	(void)ctx;
	struct opweb_client *client = NULL;

	
	opweb_log_debug("client get event:%hd\n", what);
	if (what & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) {
		client = (struct opweb_client *)ctx;
		if (!client)
			return;
		opweb_log_debug("client disconnect or server close it\n");
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

	client->t.tv_sec = 120;
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
	if (client_fd > 0)
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


void opweb_bufferevent_flush(struct bufferevent *bev)
{
	bufferevent_flush(bev, EV_READ, BEV_NORMAL);
	return;
}

int opweb_config_parse(char *conf_path)
{
	int ret = -1;
	xmlDocPtr doc = NULL;
	xmlNodePtr root = NULL;
	xmlNodePtr node;
	xmlNodePtr node1;
	struct opweb *web = NULL;
	xmlChar *value;

	if (!conf_path)
		goto out;

	web = get_h_opweb();

	doc = xmlReadFile(conf_path, "utf-8", XML_PARSE_NOBLANKS);
	if (!doc)
		goto out;

	root = xmlDocGetRootElement(doc);
	if (!root)
		goto out;
	
	for (node=root->children;node;node=node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;

		if(!xmlStrcasecmp(node->name,BAD_CAST"www_root")) {
			value = xmlNodeGetContent(node);
			strlcpy(web->www_root, (char*)value, sizeof(web->www_root));
			xmlFree(value);
		} else if(!xmlStrcasecmp(node->name,BAD_CAST"cert_root")) {

			for(node1=node->children; node1; node1=node1->next) {
				if (node1->type != XML_ELEMENT_NODE)
					continue;
				
				if(!xmlStrcasecmp(node1->name,BAD_CAST"ca")) {
					value = xmlNodeGetContent(node1);
					strlcpy(web->ca_path, (char*)value, sizeof(web->ca_path));
					xmlFree(value);
				}

				if(!xmlStrcasecmp(node1->name,BAD_CAST"key")) {
					value = xmlNodeGetContent(node1);
					strlcpy(web->key_path, (char*)value, sizeof(web->key_path));
					xmlFree(value);
				}
				
			}
		} else if (!xmlStrcasecmp(node->name,BAD_CAST"http_port")) {
			value = xmlNodeGetContent(node);
			if (!isport((char*)value)) {
				xmlFree(value);
				goto out;
			}

			web->hp_conf.port = atoi((char*)value) & 0xffff;
			xmlFree(value);
		} else if (!xmlStrcasecmp(node->name,BAD_CAST"https_port")) {
			value = xmlNodeGetContent(node);
			if (!isport((char*)value)) {
				xmlFree(value);
				goto out;
			}

			web->hps_conf.port = atoi((char*)value) & 0xffff;
			xmlFree(value);
		}
	}


	ret = 0;

out:
	if (doc) {
		xmlFreeDoc(doc);
		xmlCleanupParser();
	}

	return ret;
}




