#ifndef _LOG_JOB_H__
#define _LOG_JOB_H__

#include <libubox/list.h>
#include <pthread.h>
#include <zlog/zlog.h>



#define LOG_TEST



#define MAX_LOG_ITEM 200


struct log_item {
	struct list_head head;
	char *log;
	size_t size;
};
struct log_job {
	int sockfd;
	char ip[20];
	unsigned int port;
	void *memery_handle;
	struct event_base *base;
	struct event *event_listen;

	pthread_mutex_t log_lock; /*write log lock*/
	struct list_head log_list;
	size_t log_item_count;
	

	pthread_t pthread_write;
	pthread_rwlock_t rwlock; /*global*/
	char write_status;
	pthread_cond_t cond_consumer; /*write*/
	pthread_cond_t cond_product; /*read*/

	char log_gate[40];
	zlog_category_t *gate;

};


#define log_hmalloc(h,size) op_memery_calloc(h->memery_handle,__FILE__,__FUNCTION__,__LINE__,1,size)


#define log_hcalloc(h,nmemb,size) op_memery_calloc(h->memery_handle,__FILE__,__FUNCTION__,__LINE__,nmemb,size)


#define log_hstrdup(h,str) op_memery_strdup(h->memery_handle,__FILE__,__FUNCTION__,__LINE__, str);


#define log_hstrndup(h,str,size) op_memery_strndup(h->memery_handle,__FILE__,__FUNCTION__,__LINE__, str, size)

#define log_hrealloc(h,ptr,size) op_memery_strndup(h->memery_handle,__FILE__,__FUNCTION__,__LINE__, ptr, size)


#define log_hfree(h,ptr) op_memery_free(h->memery_handle,ptr)


struct log_job *log_job_init(void);
void log_job_exit(struct log_job *job);

void set_log_job(struct log_job *job);

struct log_job *get_log_job(void);

void log_event_read(int socket, short what, void *arg);
void *log_write_thread (void *arg);


void log_show_memery(struct log_job *job, void (*show) (void *));


#endif
