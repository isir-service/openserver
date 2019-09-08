/*
 * utils - misc libubox utility functions
 *
 * Copyright (C) 2012 Felix Fietkau <nbd@openwrt.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/mman.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <fcntl.h>
#include <pthread.h>
#include "hash.h"



struct memery_job {
	pthread_rwlock_t rwlock;
	struct op_hash_st * h;
	unsigned long long alloc;
};

#define foreach_arg(_arg, _addr, _len, _first_addr, _first_len) \
	for (_addr = (_first_addr), _len = (_first_len); \
		_addr; \
		_addr = va_arg(_arg, void **), _len = _addr ? va_arg(_arg, size_t) : 0)

#define C_PTR_ALIGN	(sizeof(size_t))
#define C_PTR_MASK	(-C_PTR_ALIGN)

void *__calloc_a(size_t len, ...)
{
	va_list ap, ap1;
	void *ret;
	void **cur_addr;
	size_t cur_len;
	int alloc_len = 0;
	char *ptr;

	va_start(ap, len);

	va_copy(ap1, ap);
	foreach_arg(ap1, cur_addr, cur_len, &ret, len)
		alloc_len += (cur_len + C_PTR_ALIGN - 1 ) & C_PTR_MASK;
	va_end(ap1);

	ptr = calloc(1, alloc_len);
	if (!ptr) {
		va_end(ap);
		return NULL;
	}

	alloc_len = 0;
	foreach_arg(ap, cur_addr, cur_len, &ret, len) {
		*cur_addr = &ptr[alloc_len];
		alloc_len += (cur_len + C_PTR_ALIGN - 1) & C_PTR_MASK;
	}
	va_end(ap);

	return ret;
}

#ifdef LIBUBOX_COMPAT_CLOCK_GETTIME
#include <mach/mach_host.h>		/* host_get_clock_service() */
#include <mach/mach_port.h>		/* mach_port_deallocate() */
#include <mach/mach_init.h>		/* mach_host_self(), mach_task_self() */
#include <mach/clock.h>			/* clock_get_time() */

static clock_serv_t clock_realtime;
static clock_serv_t clock_monotonic;

static void __constructor clock_name_init(void)
{
	mach_port_t host_self = mach_host_self();

	host_get_clock_service(host_self, CLOCK_REALTIME, &clock_realtime);
	host_get_clock_service(host_self, CLOCK_MONOTONIC, &clock_monotonic);
}

static void __destructor clock_name_dealloc(void)
{
	mach_port_t self = mach_task_self();

	mach_port_deallocate(self, clock_realtime);
	mach_port_deallocate(self, clock_monotonic);
}

int clock_gettime(int type, struct timespec *tv)
{
	int retval = -1;
	mach_timespec_t mts;

	switch (type) {
		case CLOCK_REALTIME:
			retval = clock_get_time(clock_realtime, &mts);
			break;
		case CLOCK_MONOTONIC:
			retval = clock_get_time(clock_monotonic, &mts);
			break;
		default:
			goto out;
	}

	tv->tv_sec = mts.tv_sec;
	tv->tv_nsec = mts.tv_nsec;
out:
	return retval;
}

#endif

void *cbuf_alloc(unsigned int order)
{
	char path[] = "/tmp/cbuf-XXXXXX";
	unsigned long size = cbuf_size(order);
	void *ret = NULL;
	int fd;

	fd = mkstemp(path);
	if (fd < 0)
		return NULL;

	if (unlink(path))
		goto close;

	if (ftruncate(fd, cbuf_size(order)))
		goto close;

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

	ret = mmap(NULL, size * 2, PROT_NONE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (ret == MAP_FAILED) {
		ret = NULL;
		goto close;
	}

	if (mmap(ret, size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED,
		 fd, 0) != ret ||
	    mmap(ret + size, size, PROT_READ | PROT_WRITE,
		 MAP_FIXED | MAP_SHARED, fd, 0) != ret + size) {
		munmap(ret, size * 2);
		ret = NULL;
	}

close:
	close(fd);
	return ret;
}

void cbuf_free(void *ptr, unsigned int order)
{
	munmap(ptr, cbuf_size(order) * 2);
}

int isipv4(const char *ip)
{
	int dots = 0;
	int setions = 0;

	if (NULL == ip || *ip == '.') {
		return -1;
	}

	while (*ip) {
		
		if (*ip == '.') {
			dots ++;
			if (setions >= 0 && setions <= 255) {
				setions = 0;
				ip++;
				continue;
			}
			return -1;
		}else if (*ip >= '0' && *ip <= '9') {
			setions = setions * 10 + (*ip - '0');
		} else {
			return -1;
		}
		
		ip++;
	}

	if (setions >= 0 && setions <= 255) {
		if (dots == 3) {
			return 0;
		}
	}

	return -1;
}


int isport(const char *port)
{

	int i = 0;
	int len = 0;
	int port_num = -1;

	
	if (port == NULL) {
		return -1;
	}

	len = strlen(port);
	
	if (port[0] != '+' && (port[0] < '0' || port[0] > '9')) {
		return -1;
	}
	for (i = 1; i < len; i++) {
		if (port[i] < 0 || port[i] > '9') {
			return -1;
		}
	}

	port_num = atoi(port);

	if (port_num <=0 || port_num >65535) {
		return -1;
	}
	
	return 0;
}


void op_daemon(void) 
{ 
	int pid = 0; 
	int i = 0;
	int fd = 0;
	
	pid = fork();
	if(pid > 0) {
		exit(0);
	}
	else if(pid < 0) { 
		return;
	}
 
	setsid();
 
	pid = fork();
	
	if( pid > 0) {
		exit(0);
	}
	
	else if( pid< 0) {
		return;
	}

	fd = open ("/dev/null", O_RDWR);
	if (fd < 0) {
		return;
	}
	
	dup2 (fd, 0);
	dup2 (fd, 1);
	dup2 (fd, 2);
	
	
	return;
}



unsigned long op_hash_calculation(const void *ch)
{
	char buf[40] = {};
	
	unsigned long ret = 0;
	long n;
	unsigned long v;
	int r;
	char *c;
	
    struct memery_info *info = NULL;
	
	if (ch == NULL) {
		return 0;
	}

	info = (struct memery_info *)ch;
	
	snprintf (buf,sizeof(buf), "%p", info->ptr);

	c = buf;

	
	if ((c == NULL) || (*c == '\0'))
		return (ret);

	n = 0x100;
	while (*c) {
		v = n | (*c);
		n += 0x100;
		r = (int)((v >> 2) ^ v) & 0x0f;
		ret = (ret << r) | (ret >> (32 - r));
		ret &= 0xFFFFFFFFL;
		ret ^= v * v;
		c++;
	}
	return ((ret >> 16) ^ ret);
	
}
int op_hash_compare (const void *src ,const void *dest)
{


	int status = 1;
	struct memery_info *info = NULL;
	struct memery_info *info1 = NULL; 

	if (src == NULL || dest == NULL) {
		return -1;
	}

	info = (struct memery_info *)src;
	info1 = (struct memery_info *)dest;
	if (info->ptr == info1->ptr) {
		status = 0;
	}
	fprintf (stderr,"src address:%p,dest address:%p\n", info->ptr, info1->ptr);
	return status;
}


void *op_memery_init(void)
{
	struct memery_job *job = NULL;

	job = calloc(1, sizeof(struct memery_job));
	if (job == NULL) {
		return NULL;
	}
	
	pthread_rwlock_init(&job->rwlock, NULL);
	job->h = op_hash_new(op_hash_calculation, op_hash_compare);

	if (job->h == NULL) {
		pthread_rwlock_destroy(&job->rwlock);
		free(job);
		return NULL;
	}

	return (void*)job;

}

void op_memery_exit(void *handle)
{

	return;
}


void * op_memery_calloc(void *handle, const char *file, const char *function, size_t line, size_t nmemb, size_t size)
{
	void *p = NULL;
	struct memery_info *info = NULL;

	struct memery_job * job = NULL;

	if (handle == NULL) {
		return NULL;
	}

	job = (struct memery_job *)handle;

	pthread_rwlock_wrlock(&job->rwlock);
	
	p = calloc(nmemb,size);
	if (p == NULL) {
		pthread_rwlock_unlock(&job->rwlock);
		return NULL;
	}
	

	info = calloc (1, sizeof(struct memery_info));

	if (info == NULL) {
		free(p);
		pthread_rwlock_unlock(&job->rwlock);
		return NULL;
	}

	info->ptr = p;
	info->file = strdup(file);
	info->function = strdup(function);
	info->line = line;
	info->alloc = nmemb * size;

	job->alloc += info->alloc;
	op_hash_insert(job->h, info);

	
	pthread_rwlock_unlock(&job->rwlock);
	
	return (p);

}
void *op_memery_strdup(void *handle, const char *file, const char *function, size_t line, const char *s)
{
	void *p = NULL;
	struct memery_info *info = NULL;

	struct memery_job * job = NULL;

	if (handle == NULL || s == NULL) {
		return NULL;
	}

	job = (struct memery_job *)handle;

	pthread_rwlock_wrlock(&job->rwlock);
	
	p = strdup(s);
	if (p == NULL) {
		pthread_rwlock_unlock(&job->rwlock);
		return NULL;
	}
	

	info = calloc (1, sizeof(struct memery_info));

	if (info == NULL) {
		free(p);
		pthread_rwlock_unlock(&job->rwlock);
		return NULL;
	}

	info->ptr = p;
	info->file = strdup(file);
	info->function = strdup(function);
	info->line = line;
	info->alloc = strlen(s);

	job->alloc += info->alloc;
	op_hash_insert(job->h, info);
	
	pthread_rwlock_unlock(&job->rwlock);
	
	return (p);

}
void *op_memery_strndup(void *handle, const char *file, const char *function, size_t line, const char *s, size_t n)
{
	void *p = NULL;
	struct memery_info *info = NULL;

	struct memery_job * job = NULL;

	if (handle == NULL || s == NULL) {
		return NULL;
	}

	job = (struct memery_job *)handle;

	pthread_rwlock_wrlock(&job->rwlock);
	
	p = strndup(s, n);
	if (p == NULL) {
		pthread_rwlock_unlock(&job->rwlock);
		return NULL;
	}
	

	info = calloc (1, sizeof(struct memery_info));

	if (info == NULL) {
		free(p);
		pthread_rwlock_unlock(&job->rwlock);
		return NULL;
	}

	info->ptr = p;
	info->file = strdup(file);
	info->function = strdup(function);
	info->line = line;
	info->alloc = n;

	job->alloc += info->alloc;
	op_hash_insert(job->h, info);
	
	pthread_rwlock_unlock(&job->rwlock);
	
	return (p);

}
void *op_memery_realloc(void *handle, const char *file, const char *function, size_t line, void *ptr, size_t size)
{
	void *p = NULL;
	size_t size_cal = 0;
	struct memery_info *info = NULL;
	struct memery_info info_tmp ;

	struct memery_job * job = NULL;

	if (handle == NULL) {
		return NULL;
	}

	job = (struct memery_job *)handle;

	pthread_rwlock_wrlock(&job->rwlock);
	info_tmp.ptr = ptr;
	info = op_hash_retrieve(job->h, &info_tmp);
	if (info == NULL) {
		pthread_rwlock_unlock(&job->rwlock);
		return NULL;
	}

	job->alloc -= info->alloc;
	size_cal = info->alloc;

	op_hash_delete(job->h, info);
	if(info->file){
		free(info->file);
	}

	if(info->function) {
		free(info->function);
	}

	free(info);
	
	p = realloc(ptr, size);
	if (p == NULL) {
		pthread_rwlock_unlock(&job->rwlock);
		return NULL;
	}
	

	info = calloc (1, sizeof(struct memery_info));

	if (info == NULL) {
		free(p);
		pthread_rwlock_unlock(&job->rwlock);
		return NULL;
	}
	
	size_cal += size;

	info->ptr = p;
	info->file = strdup(file);
	info->function = strdup(function);
	info->line = line;
	info->alloc = size_cal;

	job->alloc += info->alloc;
	op_hash_insert(job->h, info);
	
	pthread_rwlock_unlock(&job->rwlock);
	
	return (p);
}


void op_memery_free(void *handle, void *ptr)
{




	struct memery_job * job = NULL;
	struct memery_info * info = NULL;
	struct memery_info info_tmp;
	pthread_rwlock_t *lock = NULL;

	
	if(handle == NULL || ptr == NULL) {
		return;
	}
	
	job = (struct memery_job *)handle;

	pthread_rwlock_wrlock(&job->rwlock);
	lock = &job->rwlock;
	info_tmp.ptr = ptr;
	
	printf (" free address:%p\n",info_tmp.ptr);
	info = op_hash_retrieve(job->h, &info_tmp);
	if (info == NULL) {
		
		printf (" can not find:%p\n",info_tmp.ptr);
		pthread_rwlock_unlock(&job->rwlock);
		return;
	}


	job->alloc -= info->alloc;

	op_hash_delete(job->h, info);

	if(info->file){
		free(info->file);
	}

	if(info->function) {
		free(info->function);
	}

	free(info);
	
	pthread_rwlock_unlock(lock);

	return;
	
}


struct event_base *op_event_base_new(void)
{
	struct event_base *base = NULL;
	struct event_config *config = NULL;
	const char **methods = NULL;
	int i = 0;
	

	config = event_config_new();
	if (config == NULL) {
		return NULL;
	}

	methods = event_get_supported_methods();

	i= 0;
	while (methods != NULL && methods[i] != NULL) {
		if (strcmp(methods[i],"epoll")) {
			event_config_avoid_method(config, methods[i]);
		}
		
		i++;
	}

	base = event_base_new_with_config(config);

	event_config_free(config);

	return base;


}


unsigned int ipv4touint(const char *str_ip)
{
	struct in_addr addr;
	unsigned int int_ip = 0;
	
	if(inet_aton(str_ip,&addr)) {
		int_ip = ntohl(addr.s_addr);
	}
	
	return int_ip;
}

char * uinttoipv4(unsigned int ip)
{

	struct in_addr addr;

	char *paddr = NULL;

	addr.s_addr = htonl(ip); 

	paddr = inet_ntoa(addr);

	return paddr;
}



void op_memery_info(void *handle,void (*show) (void *))
{

	struct memery_job * job = NULL;
	if (handle == NULL) {
		return;
	}

	job = (struct memery_job *)handle;
	
	pthread_rwlock_rdlock(&job->rwlock);
	op_hash_doall(job->h, show);
	pthread_rwlock_unlock(&job->rwlock);
	return ;
}


