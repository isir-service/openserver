#include "bus.h"
#include "libubox/usock.h"
#include "module.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "event.h"
#include <signal.h>
#include "event2/bufferevent.h"

/*[0x0a:1][0x0d"1][request:1][from module:4][from_sub_type:4][to module:4][to sub_type:4][payload_size:4] 23*/
/*[0x0a:1][0x0d"1][response:1][from module:4][from_sub_type:4][to module:4][to sub_type:4][payload_size:4] 23*/

enum {
	BUS_RECV_HEADER = 1,
	BUS_RECV_DATA,
};

struct bus_header {
	unsigned int type;
	unsigned int from_module;
	unsigned int from_sub_type;
	unsigned int to_module;
	unsigned int to_sub_type;
	unsigned int payload_size;
};
struct bus_h {
	int fd;
	unsigned int module;
	bus_read_cb cb;
	//bus_recv_response response;
	bus_disconnect disconnect;
	struct bufferevent *buffer;
	struct evbuffer *evbuff;
	size_t water_level;
	unsigned char recv_buf[BUS_RCV_BUF_MAX];
	unsigned int read_index;
	int recv_type;
	void *arg;
	struct bus_header header;
	struct event_base *base;
	pthread_t thread_id;
	pthread_cond_t cond;
	pthread_mutex_t lock;
	int run;
	
};

static void bus_event_flush(struct bufferevent *bev, struct evbuffer *ev, struct bus_h *h)
{
	size_t size  = 0;
	size = evbuffer_get_length(ev);

	while(size > 0) {
		bufferevent_read(bev, h->recv_buf, sizeof(h->recv_buf));
		size = evbuffer_get_length(ev);
	}

	return;
}

static void bus_read(struct bufferevent *bev, void *ctx)
{
	size_t size_len = 0;
	size_t size_read = 0;
	unsigned int from_module =0;
	unsigned int from_sub_id = 0;
	unsigned int to_module = 0;
	unsigned int to_sub_id = 0;
	unsigned char bus_type = 0;
	unsigned int payload_size  = 0;
	struct bus_h *h = (struct bus_h*)ctx;
	if (!ctx)
		return;

	h->evbuff = bufferevent_get_input(bev);
	if (!h->evbuff)
		goto out;
	
	size_len = evbuffer_get_length(h->evbuff);
	if (size_len < h->water_level)
		goto out;

recv_data:
	size_read = bufferevent_read(bev, h->recv_buf+h->read_index, h->water_level);
	if (size_read != h->water_level)
		goto out;

	
	if (h->recv_type == BUS_RECV_HEADER) {
	
		bus_type = h->recv_buf[2];

		from_module = *((unsigned int*)(&h->recv_buf[3]));
		from_module = ntohl(from_module);
		from_sub_id = *((unsigned int*)(&h->recv_buf[7]));
		from_sub_id = ntohl(from_sub_id);

		to_module = *((unsigned int*)(&h->recv_buf[11]));
		to_module = ntohl(to_module);
		to_sub_id = *((unsigned int*)(&h->recv_buf[15]));
		to_sub_id = ntohl(to_sub_id);

		payload_size = *((unsigned int*)(&h->recv_buf[19]));
		payload_size = ntohl(payload_size);

		if (to_module != h->module)
			goto out;

		h->read_index = 23;
		h->recv_type = BUS_RECV_DATA;
		h->water_level = payload_size;
		
		h->header.from_module = from_module;
		h->header.from_sub_type = from_sub_id;
		
		h->header.to_module = to_module;
		h->header.to_sub_type = to_sub_id;
		h->header.type = bus_type;
		h->header.payload_size = payload_size;

		bufferevent_setwatermark(bev, EV_READ, h->water_level, 0);
		if (size_read + payload_size == size_len)
			goto recv_data;

		return;
		
	} else if (h->recv_type == BUS_RECV_DATA) {
		if (h->cb) {
			h->cb(h, h->header.from_module, h->header.from_sub_type, h->header.to_sub_type, (void*)(h->recv_buf+23), h->header.payload_size, h->arg);
		}
		h->water_level = 23;
		bufferevent_setwatermark(bev, EV_READ, h->water_level, 0);
		h->read_index = 0;
		h->recv_type = BUS_RECV_HEADER;

		goto out;
	} else {
		h->water_level = 23;
		h->read_index = 0;
		h->recv_type = BUS_RECV_HEADER;
		goto out;
	}

out:
	memset(h->recv_buf, 0, sizeof(h->recv_buf));
	bus_event_flush(bev, h->evbuff, h);
	return;
}
static void bus_event(struct bufferevent *bev, short what, void *ctx)
{
	struct bus_h *h = (struct bus_h*)ctx;
	if (!ctx)
		return;

	if (what & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) {
		close(h->fd);
		bufferevent_free(bev);
		bev = NULL;
		if (h->disconnect)
			h->disconnect(h, h->arg);
	}
	return;
}

static void *bus_thread_job(void *arg)
{

	struct bus_h *h = (struct bus_h *)arg;
	
	if (!arg)
		goto out;

	h->run  = 1;

	while(h->run) {
		event_base_loop(h->base,EVLOOP_NO_EXIT_ON_EMPTY);
		pthread_mutex_lock(&h->lock);
		pthread_mutex_unlock(&h->lock);
	}

	out:
		pthread_join(pthread_self() , NULL);
		return NULL;
}

void *bus_connect(unsigned int module, bus_read_cb cb, bus_disconnect disconnect, void *arg)
{
	struct bus_h *h = NULL;
	int fd = 0;
	char buf[420] = {};
	unsigned int module_send = 0;
	int poll_ret = -1;
	int read_size = 0;
	if (module >= MODULE_MAX)
		goto out;

	h = calloc(1, sizeof(struct bus_h));
	if (!h)
		goto out;

	module_send = htonl(module);

	fd = usock(USOCK_TCP,"127.0.0.1", usock_port(55556));
	if (fd < 0)
		goto out;

	h->module = module;
	h->fd = fd;
	h->disconnect = disconnect;
	h->cb = cb;
	h->arg = arg;

	memcpy(buf, &module_send, sizeof(unsigned int));
	memcpy(buf+4, "helloworld_opbus", strlen("helloworld_opbus"));
	if (write(fd,buf,4+strlen("helloworld_opbus")) < 0)
		goto out;

	poll_ret = usock_wait_ready(fd, 1000);

	if (poll_ret <= 0)
		goto out;

	memset(buf,0, sizeof(buf));
	if ((read_size = read(fd, buf, sizeof(buf))) < 0)
		goto out;

	if (strncmp(buf,"ok",2))
		goto out;

	h->base = event_base_new();
	if (!h->base)
		goto out;
	
	h->buffer = bufferevent_socket_new(h->base, fd, 0);
	if (!h->buffer)
		goto out;

	bufferevent_setcb(h->buffer, bus_read, NULL, bus_event, h);
	h->water_level = 23;
	bufferevent_setwatermark(h->buffer, EV_READ, h->water_level, 0);
	bufferevent_enable(h->buffer, EV_READ);
	h->recv_type = BUS_RECV_HEADER;

	if (pthread_create(&h->thread_id, NULL, bus_thread_job, h))
		goto out;

	pthread_cond_init(&h->cond,NULL);
	pthread_mutex_init(&h->lock,NULL);
	h->run = 1;
	return h;
	
out:

	bus_exit(h);
	return NULL;
}

void bus_exit(void *h)
{
	(void)h;

	return;
}

int bus_send(void *h,unsigned int from_sub_id,  unsigned int to_module, unsigned int to_sub_id, void *data, unsigned int size)
{
	struct bus_h *hb = (struct bus_h *)h;
	unsigned int from_module_tmp;
	unsigned int from_sub_id_tmp;
	unsigned int to_module_tmp;
	unsigned int to_sub_id_tmp;
	unsigned int size_tmp;
	unsigned int size_send = 0;

	if (!hb)
		goto out;

	if (!size || !data)
		size_send = 0;
	else
		size_send = size;


	hb->recv_buf[0] = 0x0a;
	hb->recv_buf[1] = 0x0d;
	hb->recv_buf[2] = 0;
	
	from_module_tmp = htonl(hb->module);
	from_sub_id_tmp = htonl(from_sub_id);
	to_module_tmp = htonl(to_module);
	to_sub_id_tmp = htonl(to_sub_id);
	size_tmp = htonl(size_send);

	pthread_mutex_lock(&hb->lock);
	memcpy(&hb->recv_buf[3], &from_module_tmp, sizeof(unsigned int));
	memcpy(&hb->recv_buf[7], &from_sub_id_tmp, sizeof(unsigned int));
	memcpy(&hb->recv_buf[11], &to_module_tmp, sizeof(unsigned int));
	memcpy(&hb->recv_buf[15], &to_sub_id_tmp, sizeof(unsigned int));
	memcpy(&hb->recv_buf[19], &size_tmp, sizeof(unsigned int));
	memcpy(&hb->recv_buf[23], data, size_send);
	bufferevent_write(hb->buffer, (void*)hb->recv_buf, size_send+23);
	pthread_mutex_unlock(&hb->lock);
	return 0;
	
out:
	return -1;
}

int bus_send_sync(void *h,unsigned int from_sub_id,  unsigned int to_module, unsigned int to_sub_id, void *data, unsigned int size, void*res, unsigned int res_size, unsigned int *res_actual, unsigned int ms)
{
	struct bus_h *hb = (struct bus_h *)h;
	unsigned int from_module_tmp;
	unsigned int from_sub_id_tmp;
	unsigned int to_module_tmp;
	unsigned int to_sub_id_tmp;
	unsigned int size_tmp;
	unsigned int actual = 0;
	int sock_fd;
	int poll_ret = -1;
	unsigned int size_send = 0;
	unsigned int mem_size  = 0;
	(void)sock_fd;
	(void)poll_ret;
	(void)res;
	(void)res_size;
	(void)ms;
	if (!hb)
		return -1;

	if (!size || !data)
		size_send = 0;
	else
		size_send = size;

	memset(res, 0, res_size);

	hb->recv_buf[0] = 0x0a;
	hb->recv_buf[1] = 0x0d;
	hb->recv_buf[2] = 0;
	
	from_module_tmp = htonl(hb->module);
	from_sub_id_tmp = htonl(from_sub_id);
	to_module_tmp = htonl(to_module);
	to_sub_id_tmp = htonl(to_sub_id);
	size_tmp = htonl(size_send);
	
	pthread_mutex_lock(&hb->lock);
	memcpy(&hb->recv_buf[3], &from_module_tmp, sizeof(unsigned int));
	memcpy(&hb->recv_buf[7], &from_sub_id_tmp, sizeof(unsigned int));
	memcpy(&hb->recv_buf[11], &to_module_tmp, sizeof(unsigned int));
	memcpy(&hb->recv_buf[15], &to_sub_id_tmp, sizeof(unsigned int));
	memcpy(&hb->recv_buf[19], &size_tmp, sizeof(unsigned int));
	memcpy(&hb->recv_buf[23], data, size_send);
	
	sock_fd = bufferevent_getfd(hb->buffer);
	event_base_loopbreak(bufferevent_get_base(hb->buffer));
	bufferevent_write(hb->buffer, (void*)hb->recv_buf, size_send+23);
	
	poll_ret = usock_wait_ready(sock_fd, ms);

	if (poll_ret <= 0)
		goto out;
	
	read(sock_fd, hb->recv_buf, sizeof(hb->recv_buf));
	actual = ntohl(*((unsigned int *)(&hb->recv_buf[19])));

	mem_size = actual > res_size?res_size:actual;

	if (res)
		memcpy(res, &hb->recv_buf[23], mem_size);

	if (res_actual)
		*res_actual = actual;
	
	pthread_mutex_unlock(&hb->lock);
	return 0;

out:
	pthread_mutex_unlock(&hb->lock);
	return -1;
}


