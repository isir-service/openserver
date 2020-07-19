#include "log.h"
#include <unistd.h>
#include "libxml/tree.h"
#include "libubox/utils.h"
#include "libxml/parser.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LOG_COLOR_NONE "\033[m"
#define LOG_COLOR_HEI "\033[0;30m"
#define LOG_COLOR_HONG "\033[0;31m"
#define LOG_COLOR_LV "\033[0;32m"
#define LOG_COLOR_HUANG "\033[0;33m"
#define LOG_COLOR_LAN "\033[0;34m"
#define LOG_COLOR_ZI "\033[0;35m"
#define LOG_COLOR_QING "\033[0;36m"
#define LOG_COLOR_BAI "\033[0;37m"


enum LOG_LEVEL {
	LEVEL_ERROR,
	LEVEL_WARN,
	LEVEL_INFO,
	LEVEL_DEBUG,
	LEVEL_MAX,
};


struct level_name_map {
	char *name;
	char *color;
};




struct level_name_map level_map[LEVEL_MAX] = {
	[LEVEL_ERROR] = {.name = "error", .color = LOG_COLOR_HONG},
	[LEVEL_WARN] = {.name = "warn ", .color = LOG_COLOR_HUANG},
	[LEVEL_INFO] = {.name = "info ", .color = LOG_COLOR_LV},
	[LEVEL_DEBUG] = {.name = "debug", .color = LOG_COLOR_BAI},
};

struct log_module {
	struct sockaddr_in send_addr;
	int send_fd;
	unsigned int module;
	char module_name[120];
	int pid;
	char buf[LOG_BUF_MAX];
	int enable;
	pthread_mutex_t lock;
};

static int get_conf_port(char*file_name)
{
	int ret = -1;
	xmlDocPtr doc = NULL;
	xmlNodePtr root = NULL;
	xmlNodePtr node;
	xmlChar *value;
	if (!file_name)
		goto out;

	doc = xmlReadFile(file_name, "utf-8", XML_PARSE_NOBLANKS);
	if (!doc)
		goto out;

	root = xmlDocGetRootElement(doc);
	if (!root)
		goto out;
	
	for (node=root->children;node;node=node->next) {
		if (node->type != XML_ELEMENT_NODE)
			continue;
		
		if(!xmlStrcasecmp(node->name,BAD_CAST"port")) {
			value = xmlNodeGetContent(node);
			if (!isport((char*)value)) {
				xmlFree(value);
				goto out;

			}

			ret = atoi((char*)value) & 0xffff;

			xmlFree(value);
		}

	}


out:
	if (doc) {
		xmlFreeDoc(doc);
		xmlCleanupParser();
	}

	return ret;
}


void *log_init(unsigned int module)
{
	struct log_module *m = NULL;
	int send_port = 0;

	if (module >= MODULE_MAX)
		goto out;

	m = calloc(1, sizeof(struct log_module));
	if (!m)
		goto out;

	m->module = module;
	m->pid = getpid();
	strlcpy(m->module_name, module_id_to_name(module), sizeof(m->module_name));
	m->send_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (m->send_fd <= 0) {
		free(m);
		m = NULL;
		goto out;
	}

	m->send_addr.sin_family = AF_INET;
	m->send_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	//inet_aton("127.0.0.1", &m->send_addr.sin_addr);
	send_port = get_conf_port(LOG_CONF_PATH);
	if (send_port <= 0) {
		free(m);
		m= NULL;
		goto out;
	}
	
	pthread_mutex_init(&m->lock, NULL);
	m->send_addr.sin_port = htons(send_port);
	m->enable = 1;
out:
	return m;
}

void log_exit(void *h)
{
	if (!h)
		return;

	free(h);

	return;
}

static void log_write(struct log_module *m, char *buf, unsigned int size)
{

	sendto(m->send_fd, buf, size, 0, (struct sockaddr *)&m->send_addr, sizeof(m->send_addr));

	return;
}

static int log_format(struct log_module *m, int level)
{
	time_t ti;
	struct tm *t;
	struct timeval t2;

	size_t size = 0;
	
	ti = time(NULL);

	t = localtime(&ti);
	if (!t)
		return -1;

	gettimeofday(&t2, NULL );
	*((unsigned int*)m->buf) = htonl(m->module);

	size = snprintf(m->buf+sizeof(unsigned int), sizeof(m->buf)-sizeof(unsigned int), "%s%d-%02d-%02d %02d:%02d:%02d:%06lu %s %d %s        ",
		level_map[level].color,t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, t2.tv_usec,
		m->module_name, m->pid, level_map[level].name);

		return size+sizeof(unsigned int);
}

#if 0
static void log_print(int level, void *h, const char *fmt, ...)
{
	int size = 0;
	int size2  = 0;
	struct log_module *m = NULL;
	m = (struct log_module *)h;

	if (!h)
		return;

	if (!m->enable)
		return;

	pthread_mutex_lock(&m->lock);
	size = log_format(m, level);
	if (size < 0) {
		pthread_mutex_unlock(&m->lock);
		return;
	}

	va_list args;
	va_start(args, fmt);
	size2 = vsnprintf(m->buf + size, sizeof(m->buf) - size, fmt, args);
	va_end(args);

	log_write(m, m->buf, size+size2);
	pthread_mutex_unlock(&m->lock);

	return;


}

#endif

void log_err(void *h, const char *fmt, ...)
{
	
	int size = 0;
	int size2  = 0;
	struct log_module *m = NULL;
	m = (struct log_module *)h;

	if (!h)
		return;

	if (!m->enable)
		return;

	pthread_mutex_lock(&m->lock);
	size = log_format(m, LEVEL_ERROR);
	if (size < 0) {
		pthread_mutex_unlock(&m->lock);
		return;
	}

	va_list args;
	va_start(args, fmt);
	size2 = vsnprintf(m->buf + size, sizeof(m->buf) - size, fmt, args);
	va_end(args);

	log_write(m, m->buf, size+size2);
	pthread_mutex_unlock(&m->lock);

	return;
}
void log_warn(void *h, const char *fmt, ...)
{
	
	int size = 0;
	int size2  = 0;
	struct log_module *m = NULL;
	m = (struct log_module *)h;

	if (!h)
		return;

	if (!m->enable)
		return;

	pthread_mutex_lock(&m->lock);
	size = log_format(m, LEVEL_WARN);
	if (size < 0) {
		pthread_mutex_unlock(&m->lock);
		return;
	}

	va_list args;
	va_start(args, fmt);
	size2 = vsnprintf(m->buf + size, sizeof(m->buf) - size, fmt, args);
	va_end(args);

	log_write(m, m->buf, size+size2);
	pthread_mutex_unlock(&m->lock);

	return;

}
void log_info(void *h, const char *fmt, ...)
{
	int size = 0;
	int size2  = 0;
	struct log_module *m = NULL;
	m = (struct log_module *)h;

	if (!h)
		return;

	if (!m->enable)
		return;

	pthread_mutex_lock(&m->lock);
	size = log_format(m, LEVEL_INFO);
	if (size < 0) {
		pthread_mutex_unlock(&m->lock);
		return;
	}

	va_list args;
	va_start(args, fmt);
	size2 = vsnprintf(m->buf + size, sizeof(m->buf) - size, fmt, args);
	va_end(args);
	log_write(m, m->buf, size+size2);
	printf("%s",LOG_COLOR_NONE);
	pthread_mutex_unlock(&m->lock);

	return;



}
void log_debug(void *h, const char *fmt, ...)
{
	int size = 0;
	int size2  = 0;
	struct log_module *m = NULL;
	m = (struct log_module *)h;

	if (!h)
		return;

	if (!m->enable)
		return;

	pthread_mutex_lock(&m->lock);
	size = log_format(m, LEVEL_DEBUG);
	if (size < 0) {
		pthread_mutex_unlock(&m->lock);
		return;
	}

	va_list args;
	va_start(args, fmt);
	size2 = vsnprintf(m->buf + size, sizeof(m->buf) - size, fmt, args);
	va_end(args);

	log_write(m, m->buf, size+size2);
	pthread_mutex_unlock(&m->lock);

	return;


}

int get_log_fd(void *h)
{
	struct log_module *m = NULL;
	m = (struct log_module *)h;
	if (!m)
		return -1;

	return m->send_fd;
}
