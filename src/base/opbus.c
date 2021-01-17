#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#include "opbus_type.h"

#include "opbus.h"
#include "iniparser.h"
#include "opbox/usock.h"
#include "opbox/utils.h"
#include "event.h"
#include "config.h"
#include "opbox/list.h"

struct _bus_client {
	struct list_head list;
	int fd;
	struct event *ev;
	unsigned char buf_recv[_BUS_BUF_RECV_SIZE];
};

struct _bus_socket {
	int sock_fd;
	char ip[16];
	unsigned short port;
};

struct _bus_job_thread {
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;
	struct event_base *base;
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	struct list_head list;
};

struct _bus_thread_ 
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;
};

struct _bus_job_ {

	pthread_mutex_t req_lock;
	pthread_mutexattr_t req_attr;
	pthread_mutex_t response_lock;
	pthread_mutexattr_t response_attr;
	unsigned char buf_req[_BUS_BUF_REQ_SIZE];
	unsigned char buf_response[_BUS_BUF_RESPONSE_SIZE];
	opbus_cb cb[opbus_max];
	struct _bus_job_thread job_thread;
};

struct _opbus_struct {
	struct _bus_socket sock;
	struct _bus_thread_ thread;
	struct event_base *base;
	struct event *ev;
	struct _bus_job_ job;
};

static struct _opbus_struct *self = NULL;

static void opbus_ntohl_head(struct _bus_req_head *head)
{
	head->type = ntohl(head->type);
	return;
}

static void opbus_job_thread(evutil_socket_t fd,short what,void* arg)
{
	int ret = 0;
	struct _bus_client *client = NULL;
	struct _bus_req_head *req_head = NULL;
	struct _bus_response_head *response_head = NULL;
	client = (struct _bus_client *)arg;
	int len = 0;

	ret = read(fd, client->buf_recv, _BUS_BUF_REQ_SIZE);
	if (ret < 0) {
		printf ("%s %d read faild[%d]\n",__FILE__,__LINE__, errno);
		goto out;
	}
	
	if (what == EV_READ && !ret) {
			close(client->fd);
			pthread_mutex_lock(&self->job.job_thread.lock);
			list_del(&client->list);
			pthread_mutex_unlock(&self->job.job_thread.lock);
			event_free(client->ev);
			free(client);
			goto out;
	}

	if (ret < (int)sizeof(struct _bus_req_head)) {
		printf ("%s %d read len less[%d]\n",__FILE__,__LINE__, ret);
		goto out;
	}

	req_head = (struct _bus_req_head *)client->buf_recv;
	opbus_ntohl_head(req_head);
	if (req_head->type >= opbus_max) {
		printf ("%s %d head type [%d] failed\n",__FILE__,__LINE__, req_head->type);
		goto out;
	}

	pthread_mutex_lock(&self->job.response_lock);
	if (!self->job.cb[req_head->type]) {
		printf ("%s %d cb [%d] failed\n",__FILE__,__LINE__, req_head->type);
		pthread_mutex_unlock(&self->job.response_lock);
		goto out;
	}

	len = self->job.cb[req_head->type](client->buf_recv+sizeof(*req_head), ret-sizeof(*req_head),
			self->job.buf_response+sizeof(*response_head), _BUS_BUF_RESPONSE_SIZE-sizeof(*response_head));
	response_head = (struct _bus_response_head *)self->job.buf_response;
	response_head->type = htonl(req_head->type);
	if (len < 0) {
		printf ("%s %d cb [%d] failed\n",__FILE__,__LINE__, req_head->type);
		len = 0;
	}

	len += sizeof(*response_head);

	if (len > _BUS_BUF_RESPONSE_SIZE) {
		printf ("%s %d len response to long[%d]\n",__FILE__,__LINE__, ret);
		len = _BUS_BUF_RESPONSE_SIZE;
	}

	if (write(fd, self->job.buf_response, len) < 0)
		printf ("%s %d write failed[%d]\n",__FILE__,__LINE__, errno);

	pthread_mutex_unlock(&self->job.response_lock);
	
out:
	return;
}

static void opbus_job(evutil_socket_t fd,short what,void* arg)
{
	int client_fd = 0;
	struct sockaddr_in addr;
	socklen_t len = 0;
	struct _bus_client *client = NULL;
	struct _opbus_struct *bus = NULL;

	bus = (struct _opbus_struct *)arg;
	if(!bus) {
		printf ("%s %d opbus_job parameter is unvalid\n",__FILE__,__LINE__);
		goto out;
	}

	client_fd = accept(fd, (struct sockaddr*)&addr, &len);
	if (client_fd < 0) {
		printf ("%s %d accept failed[%d]\n",__FILE__,__LINE__, errno);
		goto out;
	}

	client = calloc(1, sizeof(*client));
	if (!client) {
		close(client_fd);
		printf ("%s %d calloc failed[%d]\n",__FILE__,__LINE__, errno);
		goto out;
	}

	client->fd = client_fd;
	client->ev = event_new(bus->job.job_thread.base, client->fd, EV_READ|EV_PERSIST, opbus_job_thread, client);
	if (!client->ev) {
		close(client_fd);
		free(client);
		printf ("%s %d event_new failed[%d]\n",__FILE__,__LINE__, errno);
		goto out;
	}

	INIT_LIST_HEAD(&client->list);

	pthread_mutex_lock(&bus->job.job_thread.lock);
	list_add(&client->list, &bus->job.job_thread.list);
	pthread_mutex_unlock(&bus->job.job_thread.lock);

	if(event_add(client->ev, NULL) < 0) {
		close(client_fd);
		pthread_mutex_lock(&bus->job.job_thread.lock);
		list_del(&client->list);
		pthread_mutex_unlock(&bus->job.job_thread.lock);
		free(client);
		printf ("%s %d opbus event_add faild[%d]\n",__FILE__,__LINE__, errno);
		goto out;
	}

	if(pthread_kill(bus->job.job_thread.thread_id, SIGUSR1)) {
		printf ("%s %d pthread_killp[%x] faild[%d]\n",__FILE__,__LINE__, (unsigned int)bus->job.job_thread.thread_id, errno);
		goto out;
	}

out:
	return;
}

static void *opbus_routine (void *arg)
{
	if(event_base_loop(arg, EVLOOP_NO_EXIT_ON_EMPTY) < 0) {
		printf ("%s %d opbus_routine failed\n",__FILE__,__LINE__);
		pthread_detach(pthread_self());
		pthread_exit(NULL);
		goto exit;
	}

	printf ("%s %d opbus_routine exit\n",__FILE__,__LINE__);
exit:
	return NULL;
}

void *opbus_init(void)
{
	struct _opbus_struct *bus = NULL;
	dictionary *dict = NULL;
	const char *str = NULL;
	int str_int = 0;

	bus = calloc(1, sizeof(*bus));
	if (!bus) {
		printf ("%s %d opbus_init init failed[%d]\n",__FILE__,__LINE__, errno);
		goto exit;
	}

	dict = iniparser_load(OPSERVER_CONF);
	if (!dict) {
		printf ("%s %d iniparser_load faild[%s]\n",__FILE__,__LINE__, OPSERVER_CONF);
		goto exit;
	}

	if(!(str = iniparser_getstring(dict,BUS_SERVER,NULL))) {
		printf ("%s %d iniparser_getstring faild[%s]\n",__FILE__,__LINE__, BUS_SERVER);
		iniparser_freedict(dict);
		goto exit;
	}

	strlcpy(bus->sock.ip, str, sizeof(bus->sock.ip));
	if ((str_int =iniparser_getint(dict,BUS_PORT,-1)) < 0) {
		printf ("%s %d iniparser_getint faild[%s]\n",__FILE__,__LINE__, BUS_PORT);
		iniparser_freedict(dict);
		goto exit;
	}

	bus->sock.port = str_int;

	iniparser_freedict(dict);

	bus->sock.sock_fd = usock(USOCK_IPV4ONLY|USOCK_TCP|USOCK_SERVER, bus->sock.ip, usock_port(bus->sock.port));
	if (bus->sock.sock_fd < 0) {
		printf ("%s %d usock faild[%d]\n",__FILE__,__LINE__,errno);
		goto exit;
	}
	
	printf("opbus server:%s, port:%d\n", bus->sock.ip, bus->sock.port);

	INIT_LIST_HEAD(&bus->job.job_thread.list);

	if(pthread_mutexattr_init(&bus->job.req_attr)) {
		printf ("%s %d opbus pthread_mutexattr_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutex_init(&bus->job.req_lock, &bus->job.req_attr)) {
		printf ("%s %d opbus pthread_mutex_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutexattr_init(&bus->job.response_attr)) {
		printf ("%s %d opbus pthread_mutexattr_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutex_init(&bus->job.response_lock, &bus->job.response_attr)) {
		printf ("%s %d opbus pthread_mutex_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutexattr_init(&bus->thread.attr)) {
		printf ("%s %d opbus pthread_mutexattr_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutex_init(&bus->thread.lock, &bus->thread.attr)) {
		printf ("%s %d opbus pthread_mutex_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutexattr_init(&bus->job.job_thread.attr)) {
		printf ("%s %d opbus pthread_mutexattr_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutex_init(&bus->job.job_thread.lock, &bus->job.job_thread.attr)) {
		printf ("%s %d opbus pthread_mutex_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	bus->base = event_base_new();
	if (!bus->base) {
		printf ("%s %d opbus event_base_new faild\n",__FILE__,__LINE__);
		goto exit;
	}

	bus->ev = event_new(bus->base, bus->sock.sock_fd, EV_READ|EV_PERSIST, opbus_job, bus);
	if(!bus->ev) {
		printf ("%s %d opbus event_new faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(event_add(bus->ev, NULL) < 0) {
		printf ("%s %d opbus event_add faild\n",__FILE__,__LINE__);
		goto exit;
	}

	self = bus;

	if(pthread_attr_init(&bus->thread.thread_attr)) {
		printf ("%s %d opbus pthread_attr_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_create(&bus->thread.thread_id, &bus->thread.thread_attr, opbus_routine, bus->base)) {
		printf ("%s %d opbus pthread_create faild\n",__FILE__,__LINE__);
		goto exit;
	}

	bus->job.job_thread.base = event_base_new();
	if (!bus->job.job_thread.base) {
		printf ("%s %d opbus event_base_new faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_attr_init(&bus->job.job_thread.thread_attr)) {
		printf ("%s %d opbus pthread_attr_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_create(&bus->job.job_thread.thread_id, &bus->job.job_thread.thread_attr, opbus_routine, bus->job.job_thread.base)) {
		printf ("%s %d opbus pthread_create faild\n",__FILE__,__LINE__);
		goto exit;
	}


	return bus;

exit:
	opbus_exit(bus);
	return NULL;
}

void opbus_exit(void *bus)
{
	struct _opbus_struct *_bus = NULL;
	struct _bus_client *pos = NULL;
	struct _bus_client *n = NULL;
	void * retval;

	if (!bus)
		return;

	printf ("%s %d opbus exit\n",__FILE__,__LINE__);
	_bus = (struct _opbus_struct *)bus;


	if (_bus->job.job_thread.thread_id) {
		printf ("%s %d opbus event_base_loopbreak\n",__FILE__,__LINE__);
		while(event_base_loopbreak(_bus->job.job_thread.base) < 0)
			usleep(100);
		pthread_kill(_bus->job.job_thread.thread_id, SIGUSR1);
		pthread_join(_bus->job.job_thread.thread_id, &retval);
		printf ("%s %d opbus thread[%x] exit,status=%s\n",__FILE__,__LINE__, (unsigned int)_bus->job.job_thread.thread_id, (char*)retval);
		pthread_attr_destroy(&_bus->job.job_thread.thread_attr);
	}
		
	if(_bus->job.job_thread.base) {
		printf ("%s %d opbus pthread_kill [%d],break=%d\n",__FILE__,__LINE__, SIGUSR1, event_base_got_break(_bus->job.job_thread.base));

		pthread_mutex_lock(&_bus->job.job_thread.lock);
		list_for_each_entry_safe(pos, n, &_bus->job.job_thread.list, list) {
			if (pos) {
				close(pos->fd);
				list_del(&pos->list);
				event_free(pos->ev);
				free(pos);
			}
		}

		pthread_mutex_unlock(&_bus->job.job_thread.lock);

		event_base_free(_bus->job.job_thread.base);
	}

	if (_bus->thread.thread_id) {
		if (_bus->base) {
			while(event_base_loopbreak(_bus->base) < 0)
				usleep(100);
		}
		pthread_kill(_bus->thread.thread_id, SIGUSR1);
		pthread_join(_bus->thread.thread_id, &retval);
		printf ("%s %d opbus thread[%x] exit,status=%s\n",__FILE__,__LINE__, (unsigned int)_bus->thread.thread_id, (char*)retval);
		pthread_attr_destroy(&_bus->thread.thread_attr);
	}

	if(_bus->base) {
		printf ("%s %d opbus pthread_kill [%d],break=%d\n",__FILE__,__LINE__, SIGUSR1, event_base_got_break(_bus->base));
		if (_bus->ev)
			event_free(_bus->ev);

		event_base_free(_bus->base);
	}
	
	if (_bus->sock.sock_fd)
		close(_bus->sock.sock_fd);

	pthread_mutex_destroy(&_bus->job.req_lock);
	pthread_mutex_destroy(&_bus->job.response_lock);
	pthread_mutex_destroy(&_bus->thread.lock);

	pthread_mutexattr_destroy(&_bus->job.req_attr);
	pthread_mutexattr_destroy(&_bus->job.response_attr);
	pthread_mutexattr_destroy(&_bus->thread.attr);
	self = NULL;
	printf ("%s %d opbus exit over\n",__FILE__,__LINE__);

	return;
}

int opbus_register(unsigned int type, opbus_cb cb)
{
	if (type >= opbus_max) {
		printf ("%s %d opbus_register faild[type = %u]\n",__FILE__,__LINE__, type);
		goto failed;
	}

	self->job.cb[type] = cb;
	return 0;

failed:
	return -1;
}

int _opbus_send(unsigned int type, unsigned char *req, int size, unsigned char *response, int res_size)
{
	int fd = 0;
	struct _bus_req_head *head = NULL;
	struct _bus_response_head *response_head = NULL;
	int ret = 0;
	int count = 0;
	char *str = NULL;
	
	if (type >= opbus_max) {
		printf ("%s %d _opbus_send faild[type = %u, size= %d]\n",__FILE__,__LINE__, type, size);
		goto failed;
	}

	fd = usock(USOCK_IPV4ONLY|USOCK_TCP, self->sock.ip, usock_port(self->sock.port));
	if (fd < 0) {
		printf ("%s %d _opbus_send usock faild[%d]\n",__FILE__,__LINE__, errno);
		goto failed;
	}

	pthread_mutex_lock(&self->job.req_lock);
	head = (struct _bus_req_head *)self->job.buf_req;
	head->type = htonl(type);
	if (size > (int)(_BUS_BUF_REQ_SIZE-sizeof(*head))) {
		printf ("%s %d _opbus_send faild[size= %d，support= %d]\n",__FILE__,__LINE__, size, (int)(_BUS_BUF_REQ_SIZE-sizeof(*head)));
		pthread_mutex_unlock(&self->job.req_lock);
		goto failed;
	}

	if (req && size)
		memcpy(self->job.buf_req+sizeof(*head), req, size);
	if (write(fd, self->job.buf_req, sizeof(*head)+size) < 0) {
		printf ("%s %d _opbus_send write[%d]\n",__FILE__,__LINE__, errno);
		pthread_mutex_unlock(&self->job.req_lock);
		goto failed;
	}

	pthread_mutex_unlock(&self->job.req_lock);

	ret = usock_wait_ready(fd, _BUS_WAIT_MS);
	if (ret <= 0) {
		printf ("%s %d _opbus_send wait timeout[type=%u]\n",__FILE__,__LINE__, type);
		goto failed;
	}

	str = calloc(1, _BUS_BUF_RESPONSE_SIZE);
	if (!str) {
		printf ("%s %d _opbus_send calloc failed[type=%u][%d]\n",__FILE__,__LINE__, type, errno);
		goto failed;
	}

	ret = read(fd,str, _BUS_BUF_RESPONSE_SIZE);
	if (ret < 0) {
		free(str);
		printf ("%s %d _opbus_send read failed[type=%u][%d]\n",__FILE__,__LINE__, type, errno);
		goto failed;
	}

	if (ret < (int)sizeof(*response_head)) {
		printf ("%s %d ret too short[%d]\n",__FILE__,__LINE__, ret);
		free(str);
		goto failed;
	}

	response_head = (struct _bus_response_head *)str;

	if (response && res_size > 0) {
		if (res_size < (int)(ret-sizeof(*response_head)))
			printf ("%s %d _opbus_send copy truncate[respose=%d]\n",__FILE__,__LINE__, (int)(ret-sizeof(*response_head)));

		count = res_size > (int)(ret-sizeof(*response_head))? (int)(ret-sizeof(*response_head)):res_size;
		
		memcpy(response, str+sizeof(*response_head), count);
	}

	close(fd);

	free(str);

	return count;

failed:
	if (fd)
		close(fd);

	return -1;
}


