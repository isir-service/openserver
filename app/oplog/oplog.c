#include "oplog.h"
#include <assert.h>
#include "libxml/tree.h"
#include "libubox/utils.h"
#include "libxml/parser.h"
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

static struct oplog *h_oplog = NULL;
struct oplog *oplog_init(void)
{
	struct oplog * h_log = NULL;

	h_log = calloc(1, sizeof(struct oplog));
	if (!h_log)
		goto out;
out:
	set_h_oplog(h_log);
	return h_log;
}
void oplog_exit(struct oplog *h_log)
{
	if (!h_log)
		return;
	if(h_log->ebase)
		event_base_free(h_log->ebase);

	return;
}

struct oplog *get_h_oplog(void)
{
	assert(h_oplog);

	return h_oplog;
}

void set_h_oplog(struct oplog *h_log)
{
	h_oplog = h_log;
	return;
}

int oplog_parse_conf(const char *file_name)
{
	int ret = -1;
	xmlDocPtr doc = NULL;
	xmlNodePtr root = NULL;
	xmlNodePtr node;
	struct oplog *h_op = NULL;
	xmlChar *value;
	int index = 0;

	if (!file_name)
		goto out;

	h_op = get_h_oplog();

	doc = xmlReadFile(file_name, "utf-8", XML_PARSE_NOBLANKS);
	if (!doc)
		goto out;

	root = xmlDocGetRootElement(doc);
	if (!root)
		goto out;
	
	for (node=root->children;node;node=node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;

		if(!xmlStrcasecmp(node->name,BAD_CAST"path")) {
			value = xmlNodeGetContent(node);
			strlcpy(h_op->conf.root_path, (char*)value, sizeof(h_op->conf.root_path));
			xmlFree(value);
		}
		
		if(!xmlStrcasecmp(node->name,BAD_CAST"port")) {
			value = xmlNodeGetContent(node);
			if (!isport((char*)value)) {
				xmlFree(value);
				goto out;

			}

			h_op->conf.port = atoi((char*)value) & 0xffff;

			xmlFree(value);
		}
	}

	for(index = 0; index < MODULE_MAX; index++)
		strlcpy(h_op->conf.debug[index].module_name, module_id_to_name(index), sizeof(h_op->conf.debug[index].module_name));

	ret = 0;

out:
	if (doc) {
		xmlFreeDoc(doc);
		xmlCleanupParser();
	}

	return ret;
}


void oplog_read(int s, short what, void *arg)
{
	(void)what;
	struct oplog *h_op = NULL;
	struct sockaddr_in addr;
	socklen_t addr_len;
	unsigned int module = 0;

	if (s <= 0)
		return;

	h_op = (struct oplog *)arg;

	ssize_t size = 0; 
	size = recvfrom(s, h_op->buf, sizeof(h_op->buf), 0, &addr, &addr_len);
	if (size < 0)
		return;

	module = *((unsigned int*)h_op->buf);
	module = ntohl(module);

	if (module >= MODULE_MAX)
		return;

	if (h_op->conf.debug[module].valid && h_op->conf.debug[module].fd > 0)
		write(h_op->conf.debug[module].fd, h_op->buf+sizeof(unsigned int), size - sizeof(unsigned int));

	return;
}

void oplog_timer(int s, short what, void *arg)
{

	(void)s;
	(void)what;
	(void)arg;

	op_log_apply_conf();
	return;
}

void op_log_apply_conf(void)
{
	struct oplog *h_op = NULL;
	char buf[512] = {};
	char buf1[1024] = {};
	struct tm *t = NULL;
	time_t ti = 0;
	int i = 0;
	int update_date = 0;
	
	h_op = get_h_oplog();

	is_dir_exist(h_op->conf.root_path, 1);


	snprintf(buf, sizeof(buf), "%s/%s", h_op->conf.root_path, "debug");
	is_dir_exist(buf, 1);

	ti = time(NULL);

	t = localtime(&ti);
	if (!t)
		return;

	snprintf(buf1, sizeof(buf1), "%s/%d-%02d-%02d", buf, t->tm_year+1900, t->tm_mon+1, t->tm_mday);
	is_dir_exist(buf1, 1);

	if (strcmp(h_op->conf.date_path , buf1)) {
		strlcpy(h_op->conf.date_path, buf1, sizeof(h_op->conf.date_path));
		update_date = 1;
	}
	
	for (i = 0; i < MODULE_MAX;i++) {
		if (!h_op->conf.debug[i].valid)
			continue;

		snprintf(buf1, sizeof(buf1), "%s/%s", h_op->conf.date_path, h_op->conf.debug[i].module_name);

		if (access(buf1, F_OK) < 0)
			h_op->conf.debug[i].fd = open(buf1, O_CREAT|O_RDWR|O_APPEND|O_CREAT, 0666);
		else {
			if (update_date) {
				if (h_op->conf.debug[i].fd)
					close(h_op->conf.debug[i].fd);
				h_op->conf.debug[i].fd = open(buf1, O_CREAT|O_RDWR|O_APPEND|O_CREAT, 0666);
			} else {
				if (h_op->conf.debug[i].fd <= 0)
					h_op->conf.debug[i].fd = open(buf1, O_CREAT|O_RDWR|O_APPEND|O_CREAT, 0666);
			}
		}
	}
	
	return;
}


