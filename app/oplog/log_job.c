#include "log_job.h"
#include <stdlib.h>
#include <libubox/utils.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "oplog.h"

static struct log_job *_log_job = NULL;

#define LOG_SIZE (HEADE_SIZE + PAYLOAD_SIZE)
static char read_buf [LOG_SIZE];

static struct log_header *header = NULL;


#define log_malloc(size) op_memery_calloc(_log_job->memery_handle,__FILE__,__FUNCTION__,__LINE__,1,size)


#define log_calloc(nmemb,size) op_memery_calloc(_log_job->memery_handle,__FILE__,__FUNCTION__,__LINE__,nmemb,size)


#define log_strdup(str) op_memery_strdup(_log_job->memery_handle,__FILE__,__FUNCTION__,__LINE__, str);


#define log_strndup(str,size) op_memery_strndup(_log_job->memery_handle,__FILE__,__FUNCTION__,__LINE__, str, size)

#define log_realloc(ptr,size) op_memery_strndup(_log_job->memery_handle,__FILE__,__FUNCTION__,__LINE__, ptr, size)


#define log_free(ptr) op_memery_free(_log_job->memery_handle,ptr)


struct log_job *log_job_init(void)
{

	void *memery_handle = NULL;
	struct log_job *job = NULL;
	
	memery_handle= op_memery_init();
	if(memery_handle == NULL) {
		return NULL;
	}
	
	job = op_malloc(memery_handle, sizeof(struct log_job));
	if (job == NULL) {
		op_memery_exit(memery_handle);
		return NULL;
	}

	job->memery_handle = memery_handle;

	pthread_mutex_init(&job->log_lock, NULL);
	INIT_LIST_HEAD(&job->log_list);

	pthread_rwlock_init(&job->rwlock, NULL);


	pthread_cond_init(&job->cond_consumer, NULL);
	pthread_cond_init(&job->cond_product, NULL);




	return job;

}

void log_job_exit(struct log_job *job)
{
	(void)job;
	return;
}


void set_log_job(struct log_job *job)
{
	if(_log_job) {
		op_free(_log_job->memery_handle, _log_job);
	}
	_log_job = job;


}

struct log_job *get_log_job(void)
{
	assert(_log_job != NULL);

	return _log_job;


}



void log_event_read(int socket, short what, void *arg)
{
	(void)what;

	struct sockaddr_in addr;
	socklen_t len = 0;
	int read_size = 0;
	struct log_job *job = NULL;
	const char *payload = NULL;
	
	job = (struct log_job *)arg;
	if (job == NULL) {
		return;
	}
	
	len = sizeof(addr);
	
	memset (&addr, 0 , len);
	memset (read_buf, 0 , LOG_SIZE);

	read_size = recvfrom(socket, read_buf, LOG_SIZE, 0,(struct sockaddr *)&addr, &len);
	if (read_size < (int)HEADE_SIZE) {
		return;
	}

	header = (struct log_header *)read_buf;
#if 0
	item = log_calloc (1, sizeof(struct log_item));
	if (item == NULL) {
		return;
	}
	
	item->log = log_calloc(1,read_size);
	if (item->log == NULL) {
		free(item);
		return;
	}
	
	memcpy(item->log,read_buf,read_size);
	item->size = read_size;

	pthread_mutex_lock(&job->log_lock);
	while (job->log_item_count >= MAX_LOG_ITEM) {
		pthread_cond_wait(&job->cond_product, &job->log_lock); /*wait consumer*/
	}
	list_add_tail(&item->head, &job->log_list);
	job->log_item_count++;
	pthread_mutex_unlock(&job->log_lock);
	
	pthread_cond_signal (&job->cond_consumer); /*wakeup consumer*/
#endif
	payload = &read_buf[HEADE_SIZE];
	switch (header->level) {
		
		case _oplog_fatal:
			zlog_fatal(job->gate, "%-10u %-15s %s" ,ntohl(header->pid),header->module, payload);
			break;
	
		case _oplog_error:
			zlog_error(job->gate, "%-10u %-15s %s" ,ntohl(header->pid),header->module, payload);

			break;
			
		case _oplog_warn:
			zlog_warn(job->gate, "%-10u %-15s %s" ,ntohl(header->pid),header->module, payload);

			break;
			
		case _oplog_info:
			zlog_info(job->gate, "%-10u %-15s %s" ,ntohl(header->pid),header->module, payload);
			break;
			
		case _oplog_debug:
			zlog_debug(job->gate, "%-10u %-15s %s" ,ntohl(header->pid),header->module, payload);
		break;
		
		default:
		#ifdef LOG_TEST
			zlog_info(job->gate, "%s" , read_buf);
		#endif
			break;
	}


	return;

}




void *log_write_thread (void *arg)
{

	struct log_job *job = NULL;
	void *retval = NULL;
	struct log_item *ptr = NULL;
	struct log_item *tmp = NULL;

	
	job = (struct log_job *)arg;
	if (job == NULL) {
		pthread_exit(retval);
		assert(0);
		return NULL;
	}

	job->write_status = 1;
	while (job->write_status) {
		pthread_mutex_lock(&job->log_lock);

		while (job->log_item_count <= 0) {
			pthread_cond_wait(&job->cond_consumer, &job->log_lock); /*wait product*/
		}

		/*write log*/
		list_for_each_entry_safe (ptr, tmp,&job->log_list,head) {
			list_del(&ptr->head);
			printf ("read message:%s\n", ptr->log);
			log_free(ptr->log);
			log_free(ptr);
		}
		job->log_item_count--;

		pthread_mutex_unlock(&job->log_lock);
		pthread_cond_signal (&job->cond_product); /*wakeup product*/
		
	}

	
	job->write_status = 0;
	pthread_exit(retval);
	return NULL;
}

void log_show_memery(struct log_job *job, void (*show) (void *))
{
	if (job == NULL) {
		return;
	}

	op_memery_info(job->memery_handle, show);

	return;
}

