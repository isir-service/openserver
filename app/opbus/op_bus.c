#include "op_bus.h"
#include <string.h>

#include <assert.h>
#include <stdlib.h>
#include "interface/log.h"
#include <errno.h>
#include "interface/bus.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "libubox/utils.h"
#include "libubox/usock.h"

#define WAIT_HELLO_TIMEOUT_MS 1000
static struct _op_bus *self;

#define opbus_log_error(fmt...) log_err(self->log,fmt)
#define opbus_log_warn(fmt...) log_warn(self->log,fmt)
#define opbus_log_info(fmt...) log_info(self->log,fmt)
#define opbus_log_debug(fmt...) log_debug(self->log,fmt)


/*[0x0a:1][0x0d"1][request:1][from module:4][from_sub_type:4][to module:4][to sub_type:4][payload_size:4] 23*/
/*[0x0a:1][0x0d"1][response:1][from module:4][from_sub_type:4][to module:4][to sub_type:4][payload_size:4] 23*/

#define CLIENT_HEADER_WATER_LEVEL 23
#define OPBUS_HELLOWORLD "helloworld_opbus"

enum {
	OPBUS_CLIENT_RECV_TYPE_HEADER = 1,
	OPBUS_CLIENT_RECV_TYPE_PAYLOAD,
};
struct _op_bus *get_h_opbus(void)
{
	assert(self);

	return self;
}

void set_h_opbus(struct _op_bus *bus)
{
	self = bus;
	return;
}

struct _op_bus * opbus_init(void)
{
	struct _op_bus *bus = calloc(1, sizeof(struct _op_bus));
	set_h_opbus(bus);
	pthread_rwlock_init(&bus->rwlock, NULL);
	return bus;
}

void opbus_exit(struct _op_bus *bus)
{
	if (!bus)
		return;

	return;
}
static struct op_thread *get_thread(void)
{
	struct _op_bus *bus = get_h_opbus();
	unsigned int num_client = bus->thread[0].client_num;
	int index = 0;
	int i = 0;
	
	pthread_rwlock_rdlock(&bus->rwlock);
	for (i = 1; i < MAX_JOB_THREAT_NUM; i++) {
		if (bus->thread[i].client_num < num_client) {
			num_client = bus->thread[i].client_num;
			index = i;
		}
	}

	pthread_rwlock_unlock(&bus->rwlock);

	return &(bus->thread[index]);

}

void opbus_client_read(struct bufferevent *bev, void *ctx)
{
	struct _op_bus *bus = get_h_opbus();
	struct client_info *client = (struct client_info *)ctx;
	size_t size_len  = 0;
	size_t size_read  = 0;
	unsigned int from_module = 0;
	unsigned int to_module = 0;
	unsigned int from_sub_id = 0;
	unsigned int to_sub_id = 0;
	unsigned char bus_type = 0;
	unsigned int payload_size = 0;

	if (!client || !bev)
		return;

	client->evbuff = bufferevent_get_input(bev);
	if (!client->evbuff)
		goto out;
	
	size_len = evbuffer_get_length(client->evbuff);
	if (size_len < client->water_level)
		goto out;

recv_data:
	size_read = bufferevent_read(bev, client->recv_buf+client->read_index, client->water_level);
	if (size_read != client->water_level)
		goto out;

	if (client->recv_type == OPBUS_CLIENT_RECV_TYPE_HEADER) {
		if (client->recv_buf[0] != 0x0a && client->recv_buf[1] != 0x0d)
			goto out;

		bus_type = client->recv_buf[2];

		from_module = *((unsigned int*)(&client->recv_buf[3]));
		from_module = ntohl(from_module);
		from_sub_id = *((unsigned int*)(&client->recv_buf[7]));
		from_sub_id = ntohl(from_sub_id);

		to_module = *((unsigned int*)(&client->recv_buf[11]));
		to_module = ntohl(to_module);
		to_sub_id = *((unsigned int*)(&client->recv_buf[15]));
		to_sub_id = ntohl(to_sub_id);

		if (from_module >= MODULE_MAX || to_module >= MODULE_MAX)
			goto out;
			
		

		pthread_rwlock_rdlock(&bus->rwlock);
		if (!bus->client[to_module].enable) {
			pthread_rwlock_unlock(&bus->rwlock);
			goto out;
		}
		pthread_rwlock_unlock(&bus->rwlock);

		payload_size = *((unsigned int*)(&client->recv_buf[19]));
		payload_size = ntohl(payload_size);
		if (payload_size > (CLIENT_RECV_BUF_SIZE-CLIENT_HEADER_WATER_LEVEL))
			goto out;
		
		client->water_level = payload_size;
		client->recv_type = OPBUS_CLIENT_RECV_TYPE_PAYLOAD;
		client->read_index = CLIENT_HEADER_WATER_LEVEL;
		bufferevent_setwatermark(bev, EV_READ, client->water_level, 0);
		client->header.from_module = from_module;
		client->header.from_sub_id = from_sub_id;
		client->header.to_module = to_module;
		client->header.to_sub_id = to_sub_id;
		client->header.type = bus_type;
		if (size_len - size_read == client->water_level)
			goto recv_data;

		return;
	}else if (client->recv_type == OPBUS_CLIENT_RECV_TYPE_PAYLOAD) {
		
		pthread_rwlock_rdlock(&bus->rwlock);
		if (!bus->client[client->header.to_module].enable) {
			pthread_rwlock_unlock(&bus->rwlock);
			goto out;
		}
		pthread_rwlock_unlock(&bus->rwlock);

		if (client->header.from_module == client->header.to_module) {
			from_sub_id = htonl(client->header.from_sub_id);
			to_sub_id = htonl(client->header.to_sub_id);
			memcpy(&client->recv_buf[7], &to_sub_id, sizeof(unsigned int));
			memcpy(&client->recv_buf[15], &from_sub_id, sizeof(unsigned int));
		}

		opbus_log_info("recv[%s],level:%lu,from module:%u, to module:%u,%s\n", client->recv_buf+23,client->water_level, client->header.from_module, client->header.to_module, bus->client[client->header.to_module].module_name);
		bufferevent_write(bus->client[client->header.to_module].buffer, client->recv_buf, client->water_level+CLIENT_HEADER_WATER_LEVEL);

		client->water_level = CLIENT_HEADER_WATER_LEVEL;
		client->read_index = 0;
		client->recv_type = OPBUS_CLIENT_RECV_TYPE_HEADER;
		bufferevent_setwatermark(bev, EV_READ, client->water_level, 0);
		goto out;
	} else {
		client->water_level = CLIENT_HEADER_WATER_LEVEL;
		client->read_index = 0;
		client->recv_type = OPBUS_CLIENT_RECV_TYPE_HEADER;
		goto out;
	}

	out:
		return;
}

void opbus_client_event(struct bufferevent *bev, short what, void *ctx)
{
	struct client_info *client = (struct client_info *)ctx;
	struct _op_bus *bus = get_h_opbus();

	if (!client)
		return;

	if (what & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) {
		
		opbus_log_debug("buff try free:\n");
		close(client->client_fd);
		bufferevent_free(bev);
		bev = NULL;
		pthread_rwlock_wrlock(&bus->rwlock);
		client->enable = 0;
		client->thread->client_num--;
		bus->client_num--;
		pthread_rwlock_unlock(&bus->rwlock);
	}

	return;
}

void opbus_accept(evutil_socket_t fd, short what, void *arg)
{
	(void)what;
	(void)arg;

	int client_fd = 0;
	struct sockaddr_in addr;
	struct _op_bus *bus = get_h_opbus();
	socklen_t len = 0;
	memset(&addr, 0 ,sizeof(addr));
	struct op_thread *thread = NULL;
	char hello_buf[140] = {};
	unsigned int module = 0;
	int poll_ret = -1;

	if (fd < 0)
		goto out;

	client_fd = accept(fd, (struct sockaddr*)&addr, &len);
	if (client_fd <= 0) {
		opbus_log_error("accept failed:%d,%s\n",errno,strerror(errno));
		goto out;
	}

	pthread_rwlock_rdlock(&bus->rwlock);
	if (bus->client_num >= MODULE_MAX) {
		pthread_rwlock_unlock(&bus->rwlock);
		goto out;
	}
	pthread_rwlock_unlock(&bus->rwlock);

	poll_ret = usock_wait_ready(client_fd, WAIT_HELLO_TIMEOUT_MS);
	opbus_log_debug("poll ret:%d\n", poll_ret);
	if(poll_ret<= 0)
		goto out;

	if (read(client_fd, hello_buf, sizeof(hello_buf)) < 0)
		goto out;

	if (strcmp(hello_buf+4, OPBUS_HELLOWORLD))
		goto out;

	module = *((unsigned int *)hello_buf);
	module = ntohl(module);

	opbus_log_info("recv [%u][%s]\n", module, hello_buf+4);
	if (write(client_fd, "ok", 2) < 0 )
		goto out;

	pthread_rwlock_rdlock(&bus->rwlock);
	if ( module >= MODULE_MAX || bus->client[module].enable) {
		pthread_rwlock_unlock(&bus->rwlock);
		goto out;
	}
	pthread_rwlock_unlock(&bus->rwlock);

	thread = get_thread();
	if (!thread)
		goto out;

	bus->client[module].buffer = bufferevent_socket_new(thread->ebase_job, client_fd, 0);
	if (!bus->client[module].buffer)
		goto out;

	bufferevent_setcb(bus->client[module].buffer, opbus_client_read, NULL, opbus_client_event, &bus->client[module]);
	bus->client[module].water_level = CLIENT_HEADER_WATER_LEVEL;
	bufferevent_setwatermark(bus->client[module].buffer, EV_READ, bus->client[module].water_level, 0);
	bufferevent_enable(bus->client[module].buffer, EV_READ);

	memcpy(&bus->client[module].addr, &addr, sizeof(bus->client[module].addr));
	bus->client[module].client_fd = client_fd;
	bus->client[module].recv_type = OPBUS_CLIENT_RECV_TYPE_HEADER;
	strlcpy(bus->client[module].module_name, module_id_to_name(module), sizeof(bus->client[module].module_name));
	bus->client[module].header.pad1 = 0x0a;
	bus->client[module].header.pad2 = 0x0d;
	pthread_rwlock_wrlock(&bus->rwlock);
	thread->client_num++;
	bus->client_num++;
	bus->client[module].enable = 1;
	pthread_rwlock_unlock(&bus->rwlock);

	bus->client[module].thread = thread;
	evuser_trigger(thread->trigger);
	return;
out:
	if (client_fd)
		close(client_fd);
	return;
}

void *opbus_thread_job(void *arg)
{
	if (!arg)
		goto out;
	
	event_base_loop(arg,EVLOOP_NO_EXIT_ON_EMPTY);
	opbus_log_debug("thread exit\n");
out:
	pthread_join(pthread_self() , NULL);
	return NULL;
}

void opbus_trigger(int s, short what, void *arg)
{
	(void)s;
	(void)what;
	(void)arg;
	opbus_log_debug("trigger:%x\n", (unsigned int)pthread_self());
	return;
}


