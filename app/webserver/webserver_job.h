#ifndef WEBSERVER_JOB_H__
#define WEBSERVER_JOB_H__
#include <libubox/utils.h>


struct bind_info {
	unsigned int ip;
	unsigned short port_http;
	unsigned short port_https;
};
struct webserver_job {

	void *mem_handle;
	void *log_handle;
	struct bind_info bind;
};

int web_get_bind_ip(char *buf,int buf_size);

int web_get_bind_http_port(char *buf, int buf_size);

int web_get_bind_https_port(char *buf, int buf_size);
struct webserver_job *webserver_init(void);

void webserver_exit(struct webserver_job *job);

void webserver_run(struct webserver_job *job);

void webserver_set_base (struct webserver_job *handle);

struct webserver_job *webserver_get_base (void);

#endif

