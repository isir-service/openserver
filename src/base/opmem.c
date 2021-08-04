#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "opmem.h"
#include "opbox/list.h"
#include "opbox/hash.h"

#define OP_MEM_MAX_SIZE 1048576

enum OP_MEM {
	OP_MEM_ELE_NO = 0,
	OP_MEM_ELE_16,
	OP_MEM_ELE_32,
	OP_MEM_ELE_64,
	OP_MEM_ELE_128,
	OP_MEM_ELE_256,
	OP_MEM_ELE_512,
	OP_MEM_ELE_1k,
	OP_MEM_ELE_2k,
	OP_MEM_ELE_4k,
	OP_MEM_ELE_8k,
	OP_MEM_ELE_16k,
	OP_MEM_ELE_64k,
	OP_MEM_ELE_512k,
	OP_MEM_ELE_1m,
	op_MEM_ELE_SYS,
	OP_MEM_ELE_MAX,
};

struct mem_alloc_ele {
	char *name;
	size_t size;
	int num; /*default*/

	struct list_head *list;
	unsigned int can_used_num;
	unsigned int all_num;
	unsigned long long op_malloc_num;
	unsigned long long op_free_num;
};

static struct mem_alloc_ele op_mem_ele[OP_MEM_ELE_MAX] = {
	[OP_MEM_ELE_16] = {.name="16B pool",.size=16, .num=20},
	[OP_MEM_ELE_32] = {.name="32B pool",.size=32, .num=40},
	[OP_MEM_ELE_64] = {.name="64B pool",.size=64, .num=80},
	[OP_MEM_ELE_128] = {.name="128B pool",.size=128, .num=160},
	[OP_MEM_ELE_256] = {.name="256B pool",.size=256, .num=320},
	[OP_MEM_ELE_512] = {.name="512B pool",.size=512, .num=320},
	[OP_MEM_ELE_1k] = {.name="1kB pool",.size=1024, .num=320},
	[OP_MEM_ELE_2k] = {.name="2kB pool",.size=2048, .num=320},
	[OP_MEM_ELE_4k] = {.name="4kB pool",.size=4096, .num=320},
	[OP_MEM_ELE_8k] = {.name="8kB pool",.size=8192, .num=8},
	[OP_MEM_ELE_16k] = {.name="16kB pool",.size=16384, .num=8},
	[OP_MEM_ELE_64k] = {.name="64kB pool",.size=65536, .num=8},
	[OP_MEM_ELE_512k] = {.name="512kB pool",.size=524288, .num=4},
	[OP_MEM_ELE_1m] = {.name="1mB pool",.size=1048576, .num=2},
	[op_MEM_ELE_SYS] = {.name="system",.size=0, .num=0},
};

static struct op_mem_info *self = NULL;

struct op_mem_statistic {
	unsigned long long pool_alloc;
	unsigned long long pool_alloc_noused;
	unsigned long long pool_alloc_used;
	unsigned long long system_alloc;
	unsigned long long system_alloc_noused;
	unsigned long long system_alloc_used;
};

struct op_mem_info {
	pthread_mutex_t lock;
	pthread_mutexattr_t attr;
	void *hash;
	struct op_mem_statistic statistic;
};

struct op_mem_node {
	struct list_head list;
	void *ptr;
	size_t size;
	int index;
};

static struct mem_alloc_ele * op_mem_get_list(size_t size)
{
	int i = 0;
	for (i = OP_MEM_ELE_NO+1; i < OP_MEM_ELE_MAX; i++) {
		if (op_mem_ele[i].size >= size)
			return &op_mem_ele[i];
	}

	return NULL;
}

unsigned long op_mem_hash (const void *node)
{
	unsigned long long hash = 0;

	if (!node) {
		printf("op mem_hash node is null\n");
		return 0;
	}

	struct op_mem_node *p_node = (struct op_mem_node *)node;

	hash = (((unsigned long long)p_node->ptr) & 0xffffffff);
	return hash;
}

int op_mem_compare(const void *src_node, const void *dest_node)
{
	struct op_mem_node *p_src = (struct op_mem_node *)src_node;
	struct op_mem_node *p_dest = (struct op_mem_node *)dest_node;

	if (!src_node || !dest_node) {
		printf("op_mem_compare node is null\n");
		return 1;
	}

	return !(p_src->ptr == p_dest->ptr);
}

void *opmem_init(void)
{
	int i = 0, j = 0;
	void *ptr = NULL;
	
	struct list_head *mem_list = NULL;
	struct op_mem_node *mem_node = NULL;

	struct op_mem_info *mem = calloc(1, sizeof(struct op_mem_info));
	if (!mem) {
		printf("%s %d opmem init mem failed, errno = %d\n", __FILE__,__LINE__, errno);
		return NULL;
	}

	if(pthread_mutexattr_init(&mem->attr)) {
		printf ("%s %d pthread_mutexattr_init faild, errno =%d\n",__FILE__,__LINE__, errno);
		goto exit;
	}

	if(pthread_mutex_init(&mem->lock, &mem->attr)) {
		printf ("%s %d pthread_mutex_init faild, errno=%d\n",__FILE__,__LINE__, errno);
		goto exit;
	}

	for (i = OP_MEM_ELE_NO+1; i< OP_MEM_ELE_MAX; i++) {
		mem_list = calloc(1, sizeof (struct list_head));
		if (!mem_list) {
			printf("%s %d opmem init mem failed, errno = %d\n", __FILE__,__LINE__, errno);
			goto exit;
		}

		INIT_LIST_HEAD(mem_list);

		op_mem_ele[i].list = mem_list;
		if (!op_mem_ele[i].size || !op_mem_ele[i].num)
			continue;

		ptr = calloc(op_mem_ele[i].size, op_mem_ele[i].num);
		if (!ptr) {
			printf("%s %d opmem init mem failed, errno = %d\n", __FILE__,__LINE__, errno);
			goto exit;
		}

		for (j = 0; j < op_mem_ele[i].num; j++) {
			mem_node = calloc(1, sizeof(struct op_mem_node));
			if (!mem_node) {
				printf("%s %d opmem init mem failed, errno = %d\n", __FILE__,__LINE__, errno);
				goto exit;
			}
			INIT_LIST_HEAD(&mem_node->list);
			mem_node->index = j;
			mem_node->ptr = (char*)ptr+(j*op_mem_ele[i].size);
			mem_node->size = op_mem_ele[i].size;
			list_add_tail(&mem_node->list, op_mem_ele[i].list);
			op_mem_ele[i].can_used_num++;
			op_mem_ele[i].all_num++;
			mem->statistic.pool_alloc += mem_node->size;
			mem->statistic.pool_alloc_noused += mem_node->size;
		}

	}
	mem->hash = op_hash_new(op_mem_hash, op_mem_compare);
	if (!mem->hash) {
		printf("%s %d opmem init hash failed\n", __FILE__,__LINE__);
		goto exit;
	}

	self = mem;
	return mem;

exit:
	opmem_exit(mem);
	return NULL;
}

void *op_malloc(size_t size)
{
	void *ptr = NULL;
	struct op_mem_node *mem_node = NULL;
	struct mem_alloc_ele *ele = NULL;
	int i = 0;

	if (!self)
		return NULL;

	if (size > OP_MEM_MAX_SIZE) {
		ptr = malloc(size);
		if (!ptr)
			return NULL;
		mem_node = calloc(1, sizeof(struct op_mem_node));
		if (!mem_node) {
			free(ptr);
			return NULL;
		}

		mem_node->index = 0;
		mem_node->ptr = ptr;
		mem_node->size = size;
		INIT_LIST_HEAD(&mem_node->list);
		pthread_mutex_lock(&self->lock);
		op_hash_insert(self->hash, mem_node);
		list_add_tail(&mem_node->list,op_mem_ele[op_MEM_ELE_SYS].list);
		self->statistic.system_alloc += mem_node->size;
		self->statistic.system_alloc_used += mem_node->size;
		pthread_mutex_unlock(&self->lock);
		return ptr;
	}

	ele = op_mem_get_list(size);
	if (!ele)
		return NULL;

	pthread_mutex_lock(&self->lock);

	if(list_empty(ele->list)) {
		ptr = calloc(ele->size, ele->num);
		if (!ptr) {
			pthread_mutex_unlock(&self->lock);
			return NULL;
		}
		
		for (i = 0; i < ele->num; i++) {
			mem_node = calloc(1, sizeof(struct op_mem_node));
			if (!mem_node) {
				free(ptr);
				pthread_mutex_unlock(&self->lock);
				return NULL;
			}

			INIT_LIST_HEAD(&mem_node->list);
			mem_node->index = i;
			mem_node->ptr = (char*)ptr+(i*ele->size);
			mem_node->size = ele->size;
			list_add_tail(&mem_node->list, ele->list);
			ele->can_used_num++;
			ele->all_num++;
			self->statistic.pool_alloc += mem_node->size;
			self->statistic.pool_alloc_noused += mem_node->size;
		}
		
	}

	mem_node = list_first_entry(ele->list, struct op_mem_node , list);
	ele->can_used_num--;
	
	list_del_init(&mem_node->list);

	op_hash_insert(self->hash, mem_node);
	
	self->statistic.pool_alloc_noused -= mem_node->size;
	self->statistic.pool_alloc_used += mem_node->size;
	pthread_mutex_unlock(&self->lock);
	ele->op_malloc_num++;
	return mem_node->ptr;
}

void *op_calloc(size_t nmemb, size_t size)
{
	void *ptr = NULL;
	ptr = op_malloc(nmemb*size);
	if (!ptr)
		return NULL;

	memset(ptr, 0, nmemb*size);
	return ptr;
}

void *op_realloc(void *ptr, size_t size)
{
	struct op_mem_node mem_node;
	struct op_mem_node *p_node = NULL;
	void * ptr_out = NULL;
	
	if (!self)
		return NULL;
	
	mem_node.ptr = ptr;
	pthread_mutex_lock(&self->lock);
	p_node = op_hash_retrieve(self->hash, &mem_node);
	if (!p_node) {
		pthread_mutex_unlock(&self->lock);
		return NULL;
	}

	pthread_mutex_unlock(&self->lock);

	ptr_out = op_calloc(1,p_node->size+size);
	if (ptr_out)
		return NULL;

	memcpy(ptr_out, p_node->ptr, p_node->size);
	op_free(p_node->ptr);
	return ptr_out;
}

void op_free(void *ptr)
{
	struct op_mem_node mem_node;
	struct mem_alloc_ele * ele;
	struct op_mem_node *p_node = NULL;

	if (!self)
		return;

	mem_node.ptr = ptr;
	pthread_mutex_lock(&self->lock);
	p_node = op_hash_retrieve(self->hash, &mem_node);
	if (!p_node)
		goto out;

	if (p_node->size > OP_MEM_MAX_SIZE) {
		op_hash_delete(self->hash, p_node);
		list_del_init(&p_node->list);
		self->statistic.system_alloc -= p_node->size;
		self->statistic.system_alloc_used -= p_node->size;
		free(ptr);
		free(p_node);
		goto out;
	}

	ele = op_mem_get_list(p_node->size);
	if (!ele)
		goto out;

	op_hash_delete(self->hash, p_node);
	list_add_tail(&p_node->list, ele->list);
	ele->can_used_num++;
	self->statistic.pool_alloc_noused += p_node->size;
	self->statistic.pool_alloc_used -= p_node->size;
	ele->op_free_num++;

out:
	pthread_mutex_unlock(&self->lock);
	return;
}

void opmem_exit(void *mem)
{
	if (!mem)
		return;

	return;
}

int op_mem_information (char *buf, int size)
{
	int i = 0;
	int ret = 0;
	int index = 0;
	unsigned int ele_size = 0;
	unsigned int used_num = 0;

	if (!buf || size <= 0 || !self)
		return 0;

	index = 0;
	ret = snprintf(buf+index, size -index, "===============memory pool ==============\r\n");
	if (ret < 0)
		return index;

	index += ret;
	for (i = OP_MEM_ELE_NO+1; i < OP_MEM_ELE_MAX; i++) {
		if (i == op_MEM_ELE_SYS)
			continue;

		if (size - index <=0)
			return index;

		ele_size = (unsigned int)op_mem_ele[i].size;
		used_num = op_mem_ele[i].can_used_num;
		ret = snprintf(buf+index, size -index, "[%10s] ele size(B): %10u, all_num: %10d, can_used_num: %10d, total size: %10uB = %10.2lfkB=%10.2lfMB, op_malloc_num : %10llu, op_free_num:%10llu\r\n", 
				op_mem_ele[i].name, ele_size, op_mem_ele[i].all_num , used_num, ele_size*used_num, ele_size*used_num/1204.0, ele_size*used_num/1024.0/1024.0,
				op_mem_ele[i].op_malloc_num, op_mem_ele[i].op_free_num);

		if (ret < 0)
			return index;

		index += ret;
	}

	if (size - index <=0)
		return index;

	ret = snprintf(buf+index, size -index, "[%30s]: %10lluB = %10.2lfkB = %10.2lfMB\r\n","pool alloc total size",
			self->statistic.pool_alloc , self->statistic.pool_alloc/1204.0, self->statistic.pool_alloc/1024.0/1024.0);

	if (ret < 0)
		return index;

	index += ret;

	if (size - index <=0)
		return index;

	ret = snprintf(buf+index, size -index, "[%30s]: %10lluB = %10.2lfkB = %10.2lfMB\r\n","pool alloc used total size",
			self->statistic.pool_alloc_used , self->statistic.pool_alloc_used/1204.0, self->statistic.pool_alloc_used/1024.0/1024.0);
	if (ret < 0)
		return index;
	index += ret;

	if (size - index <=0)
		return index;

	ret = snprintf(buf+index, size -index, "[%30s]: %10lluB = %10.2lfkB = %10.2lfMB\r\n","pool alloc noused total size",
			self->statistic.pool_alloc_noused , self->statistic.pool_alloc_noused/1204.0, self->statistic.pool_alloc_noused/1024.0/1024.0);
	if (ret < 0)
		return index;
	index += ret;


	if (size - index <=0)
		return index;

	ret = snprintf(buf+index, size -index, "===============system memory ==============\r\n");
	if (ret < 0)
		return index;
	index += ret;

	if (size - index <=0)
		return index;

	ret = snprintf(buf+index, size -index, "[%30s] : %10lluB = %10.2lfkB = %10.2lfMB\r\n", "system alloc total size",
			self->statistic.system_alloc , self->statistic.system_alloc/1204.0, self->statistic.system_alloc/1024.0/1024.0);
	if (ret < 0)
		return index;
	index += ret;

	if (size - index <=0)
		return index;

	ret = snprintf(buf+index, size -index, "[%30s] : %10lluB = %10.2lfkB = %10.2lfMB\r\n", "system alloc used total size",
			self->statistic.system_alloc_used , self->statistic.system_alloc_used/1204.0, self->statistic.system_alloc_used/1024.0/1024.0);
	if (ret < 0)
		return index;
	index += ret;

	return index;

}



