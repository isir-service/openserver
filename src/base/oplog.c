#include "config.h"
#include "oplog.h"
#include "iniparser.h"
#include "opbox/usock.h"
#include "opbox/utils.h"

#include "event.h"
#include "opbox/list.h"

#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define LOG_COLOR_NONE "\033[m"
#define LOG_COLOR_BLACK "\033[0;30m"
#define LOG_COLOR_RED "\033[0;31m"
#define LOG_COLOR_GREEN "\033[0;32m"
#define LOG_COLOR_YELLOW "\033[0;33m"
#define LOG_COLOR_BULE "\033[0;34m"
#define LOG_COLOR_PURPLE "\033[0;35m"
#define LOG_COLOR_WHITE "\033[0;37m"

#define LOG_SERVER "oplog:log_ip"
#define LOG_PORT "oplog:log_port"

#define LOG_SEND_BUF_SIZE 4096
#define LOG_RECV_BUF_SIZE 4096
#define LOG_RECV_LIST_SIZE 1024
#define LOG_DISK_THREAD_NUM 2

struct log_level_name_map {
	char *name;
	char *color;
};

struct log_level_name_map log_level_map[] = {
	[oplog_level_error] = {.name = "error", .color = LOG_COLOR_RED},
	[oplog_level_warn] = {.name = "warn ", .color = LOG_COLOR_PURPLE},
	[oplog_level_info] = {.name = "info ", .color = LOG_COLOR_GREEN},
	[oplog_level_debug] = {.name = "debug", .color = LOG_COLOR_WHITE},
};

struct _log_thread_ 
{
	pthread_t thread_id;
	pthread_attr_t thread_attr;
	pthread_t disk_thread_id[LOG_DISK_THREAD_NUM];
	pthread_attr_t disk_thread_attr[LOG_DISK_THREAD_NUM];
	int disk_run;
};

struct _log_socket {
	int sock_fd;
	unsigned int ip;
	unsigned short port;
};

struct _log_send_header {
	char file[64];
	char function[64];
	int line;
	int level;
	int type;
};

struct _log_send_ {
	int send_fd;
	struct sockaddr_in send_addr;
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;
	struct _log_send_header head;
	unsigned char buf_send[LOG_SEND_BUF_SIZE];
};

struct _log_item_buf_ {
	unsigned char *buf;
	int size;
};

struct _log_item_
{
	struct list_head list;
	struct _log_item_buf_ log;
};

struct _log_recv_ {
	struct list_head vector; 
	int vector_num;
	pthread_cond_t cont;
	pthread_condattr_t cont_attr;
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;
	unsigned char buf_secv[LOG_RECV_BUF_SIZE];
	int disk_run;
};

struct _oplog_struct_
{
	struct _log_thread_ thread;
	struct _log_socket sock;
	struct event_base *base;
	struct event *ev;
	struct _log_send_ send;
	struct _log_recv_ recv;
};

static struct _oplog_struct_ *self;

static void oplog_job(evutil_socket_t fd,short what,void* arg)
{
	int ret = 0;
	struct sockaddr_in addr;
	socklen_t addr_len;
	unsigned char * str = NULL;
	struct _log_item_ *item = NULL;
	struct _oplog_struct_ *_op = NULL;

	_op = (struct _oplog_struct_ *)arg;
	if (!_op) {
		printf ("%s %d oplog_job handle failed\n",__FILE__,__LINE__);
		goto out;
	}

	ret = recvfrom(fd,_op->recv.buf_secv, sizeof(_op->recv.buf_secv), 0, (struct sockaddr*)&addr, &addr_len);
	if(ret < 0) {
		printf ("%s %d oplog_job read failed[%d]\n",__FILE__,__LINE__,errno);
		goto out;
	}

	if (ret > LOG_RECV_BUF_SIZE) {
		printf ("%s %d oplog_job recv message too long\n",__FILE__,__LINE__);
		goto out;
	}
	
	str = calloc(1,ret+1);
	if (!str) {
		printf ("%s %d oplog_job calloc failed[%d]\n",__FILE__,__LINE__,errno);
		goto out;
	}

	item = calloc(1, sizeof(*item));
	if (!item) {
		printf ("%s %d oplog_job calloc failed[%d]\n",__FILE__,__LINE__,errno);
		goto out;
	}

	memcpy(str, _op->recv.buf_secv, ret);

	item->log.buf = str;
	item->log.size = ret;
	INIT_LIST_HEAD(&item->list);

	pthread_mutex_lock(&_op->recv.lock);
	if (_op->recv.vector_num > LOG_RECV_LIST_SIZE) {
		printf ("%s %d oplog_job vctor is full\n",__FILE__,__LINE__);
		free(str);
		free(item);
		pthread_mutex_unlock(&_op->recv.lock);
		goto out;
	}

	list_add_tail(&item->list, &_op->recv.vector);
	_op->recv.vector_num++;
	pthread_cond_broadcast(&_op->recv.cont);
	pthread_mutex_unlock(&_op->recv.lock);
	return;
out:
	return;
}

static void *oplog_routine (void *arg)
{
	if(event_base_dispatch(arg) < 0) {
		printf ("%s %d oplog_routine failed\n",__FILE__,__LINE__);
		pthread_detach(pthread_self());
		pthread_exit(NULL);
		goto exit;
	}

	printf ("%s %d oplog_routine exit\n",__FILE__,__LINE__);
exit:
	return NULL;
}

static int _oplog_check_disk(void)
{
	return 0;
}

static void _oplog_sync_to_disk(int fd, struct _log_item_buf_ *log)
{

	return;
}

static void *oplog_disk (void *arg)
{
	struct _log_recv_ *_recv = NULL;
	struct _log_item_ *item;
	int log_fd = 0;

	_recv = (struct _log_recv_ *)arg;

	if (!_recv) {
		printf ("%s %d oplog_disk handle failed\n",__FILE__,__LINE__);
		goto exit;
	}

	_recv->disk_run = 1;

	while(_recv->disk_run) {
		pthread_mutex_lock(&_recv->lock);
		if (list_empty(&_recv->vector))
			pthread_cond_wait(&_recv->cont, &_recv->lock);

		if(list_empty(&_recv->vector))
			goto next;

		if ((log_fd = _oplog_check_disk()) < 0) {
			printf ("%s %d _oplog_check_disk failed[%x]\n",__FILE__,__LINE__, (unsigned int)pthread_self());
			goto next;
		}

		item = list_first_entry(&_recv->vector, struct _log_item_ , list);
		if (!item) {
			printf ("%s %d oplog_disk item failed[%x]\n",__FILE__,__LINE__, (unsigned int)pthread_self());
			goto next;
		}

		_oplog_sync_to_disk(log_fd, &item->log);

		list_del_init(&item->list);
		if (item->log.buf) {
			free(item->log.buf);
			item->log.buf = NULL;
		}

		free(item);
		_recv->vector_num--;
next:
		pthread_mutex_unlock(&_recv->lock);
	}
	
exit:
	printf ("%s %d oplog_disk exit[%x]\n",__FILE__,__LINE__, (unsigned int)pthread_self());
	return NULL;
}

void *oplog_init(void)
{
	struct _oplog_struct_ *_op = NULL;
	dictionary *dict = NULL;
	const char *ip = NULL;
	int port = 0;
	int i = 0;

	_op = calloc(1, sizeof(struct _oplog_struct_));
	if (!_op) {
		printf ("%s %d oplog_init failed[errno:%d]\n",__FILE__,__LINE__, errno);
		goto exit;
	}

	dict = iniparser_load(OPSERVER_CONF);
	if (!dict) {
		printf ("%s %d iniparser_load faild[%s]\n",__FILE__,__LINE__, OPSERVER_CONF);
	}

	if(!(ip = iniparser_getstring(dict,LOG_SERVER,NULL))) {
		printf ("%s %d iniparser_getstring faild[%s]\n",__FILE__,__LINE__, LOG_SERVER);
		iniparser_freedict(dict);
		goto exit;
	}
	
	if ((port =iniparser_getint(dict,LOG_PORT,-1))< 0) {
		printf ("%s %d iniparser_getint faild[%s]\n",__FILE__,__LINE__, LOG_PORT);
		iniparser_freedict(dict);
		goto exit;
	}

	_op->sock.ip = ntohl(inet_addr(ip));
	_op->sock.port = port;

	_op->sock.sock_fd = usock(USOCK_IPV4ONLY|USOCK_UDP|USOCK_SERVER, ip, usock_port(port));
	if (_op->sock.sock_fd < 0) {
		iniparser_freedict(dict);
		printf ("%s %d iniparser_getint faild[%s]\n",__FILE__,__LINE__, LOG_PORT);
	}
	
	printf("oplog server:%s, port:%d\n", ip, port);

	iniparser_freedict(dict);
	
	_op->send.send_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(_op->send.send_fd < 0) {
		printf ("%s %d socket faild[%d]\n",__FILE__,__LINE__, errno);
		goto exit;
	}
	_op->send.send_addr.sin_family = AF_INET;
	_op->send.send_addr.sin_addr.s_addr = htonl(_op->sock.ip);
	_op->send.send_addr.sin_port = htons(_op->sock.port);

	if(pthread_mutexattr_init(&_op->send.attr)) {
		printf ("%s %d oplog pthread_mutexattr_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutex_init(&_op->send.lock, &_op->send.attr)) {
		printf ("%s %d oplog pthread_mutex_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutexattr_init(&_op->recv.attr)) {
		printf ("%s %d oplog pthread_mutexattr_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_mutex_init(&_op->recv.lock, &_op->recv.attr)) {
		printf ("%s %d oplog pthread_mutex_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_condattr_init(&_op->recv.cont_attr)) {
		printf ("%s %d oplog pthread_rwlockattr_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_cond_init(&_op->recv.cont, &_op->recv.cont_attr)) {
		printf ("%s %d oplog pthread_rwlock_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	INIT_LIST_HEAD(&_op->recv.vector);

	_op->base = event_base_new();
	if (!_op->base) {
		printf ("%s %d oplog event_base_new faild\n",__FILE__,__LINE__);
		goto exit;
	}

	_op->ev = event_new(_op->base, _op->sock.sock_fd, EV_READ|EV_PERSIST, oplog_job, _op);
	if(!_op->ev) {
		printf ("%s %d oplog event_new faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(event_add(_op->ev, NULL) < 0) {
		printf ("%s %d oplog event_add faild\n",__FILE__,__LINE__);
		goto exit;
	}

	if(pthread_attr_init(&_op->thread.thread_attr)) {
		printf ("%s %d oplog pthread_attr_init faild\n",__FILE__,__LINE__);
		goto exit;
	}

	self = _op;

	if(pthread_create(&_op->thread.thread_id, &_op->thread.thread_attr, oplog_routine, _op->base)) {
		printf ("%s %d oplog pthread_create faild\n",__FILE__,__LINE__);
		goto exit;
	}
	
	for( i = 0; i < LOG_DISK_THREAD_NUM; i++) {
		if(pthread_create(&_op->thread.disk_thread_id[i], &_op->thread.disk_thread_attr[i], oplog_disk, &_op->recv)) {
			printf ("%s %d oplog pthread_create faild\n",__FILE__,__LINE__);
			goto exit;
		}
	}
	
	return _op;
exit:

	oplog_exit(_op);
	return NULL;
}

void oplog_exit(void *oplog)
{
	struct _oplog_struct_ *_op = NULL;
	int i = 0;
	void *retval;
	
	if (!oplog)
		return;

	printf ("%s %d oplog_exit\n",__FILE__,__LINE__);

	_op = oplog;

	if(_op->base) {
		
		printf ("%s %d oplog event_base_loopbreak\n",__FILE__,__LINE__);
		while(event_base_loopbreak(_op->base) < 0)
			usleep(100);
		printf ("%s %d oplog pthread_kill [%d],break=%d\n",__FILE__,__LINE__, SIGUSR1, event_base_got_break(_op->base));

		event_base_free(_op->base);
	}

	if (_op->thread.thread_id) {
		pthread_kill(_op->thread.thread_id, SIGUSR1);
		pthread_join(_op->thread.thread_id, &retval);
		printf ("%s %d oplog thread[%x] exit,status=%s\n",__FILE__,__LINE__, (unsigned int)_op->thread.thread_id, (char*)retval);
		pthread_attr_destroy(&_op->thread.thread_attr);
	}

	if(_op->recv.disk_run) {
		_op->recv.disk_run = 0;
		pthread_mutex_lock(&_op->recv.lock);
		pthread_cond_broadcast(&_op->recv.cont);
		pthread_mutex_unlock(&_op->recv.lock);
		for (i = 0; i < LOG_DISK_THREAD_NUM;i++) {
			if (_op->thread.disk_thread_id[i]) {
				pthread_join(_op->thread.disk_thread_id[i], &retval);
				printf ("%s %d oplog thread[%x] exit,status=%s\n",__FILE__,__LINE__, (unsigned int)_op->thread.disk_thread_id[i], (char*)retval);
				pthread_attr_destroy(&_op->thread.disk_thread_attr[i]);
			}
		}
	}
	
	if (_op->send.send_fd)
		close(_op->send.send_fd);

	if(_op->sock.sock_fd)
		close(_op->sock.sock_fd);

	pthread_cond_destroy(&_op->recv.cont);

	pthread_mutex_destroy(&_op->send.lock);
	
	pthread_mutex_destroy(&_op->recv.lock);

	pthread_condattr_destroy(&_op->recv.cont_attr);
	
	pthread_mutexattr_destroy(&_op->send.attr);
	
	pthread_mutexattr_destroy(&_op->recv.attr);


	free(_op);
	_op = NULL;
	self = NULL;
	return;
}

static void log_write(unsigned char *buf, unsigned int size)
{
	sendto(self->send.send_fd, buf, size, 0, (struct sockaddr *)&self->send.send_addr, sizeof(self->send.send_addr));
	return;
}

void oplog_print(int log_type, char *file, const char *function, int line, int level, const char *fmt, ...)
{
	va_list args;
	size_t size = 0;

	va_start(args, fmt);
	pthread_mutex_lock(&self->send.lock);
	strlcpy(self->send.head.file, file, sizeof(self->send.head.file));
	strlcpy(self->send.head.function, function, sizeof(self->send.head.function));
	self->send.head.line = htonl(line);
	self->send.head.level = htonl(level);
	self->send.head.type = htonl(log_type);
	memcpy(self->send.buf_send, &self->send.head, sizeof(self->send.head));
	size = vsnprintf((char*)(self->send.buf_send+sizeof(self->send.head)), LOG_SEND_BUF_SIZE-sizeof(self->send.head), fmt, args);
	log_write(self->send.buf_send,size+sizeof(self->send.head));
	pthread_mutex_unlock(&self->send.lock);
	va_end(args);

	return;
}

