#include "fastcgi.h"
#include "libubox/usock.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "opweb_pub.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


struct begin_request{
	struct fast_header head;
	struct fcgi_begin_request_body begin_body;
}__attribute__ ((aligned (1)));

struct params_request {
	struct fast_header head;
	struct fcgi_params_body params_body;
}__attribute__ ((aligned (1)));

struct fcgi_handle {
	int fd;
	unsigned short id;
	struct begin_request begin;
	struct params_request params;
	unsigned char content[HTTPS_CLIENT_READ_BUF];
	unsigned char read_buf[65535];
	unsigned short rd_size;
};

struct fcgi_handle fcgi_h[1];

void *fastcgi_connect(char *server, unsigned short port)
{
	

	int fd = 0;
	struct sockaddr_in server_address;
	int ret = 0;
	
	if (!server || !port)
		return NULL;

	memset(&server_address, 0, sizeof(server_address));
	
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd <= 0)
		return NULL;

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr(server);
	server_address.sin_port = htons(port);

	ret = connect(fd, (struct sockaddr *)&server_address, sizeof(server_address));
	if (ret < 0)
		return NULL;
	
	memset(&fcgi_h[0], 0, sizeof(struct fcgi_handle));
	
	fcgi_h[0].fd = fd;
	fcgi_h[0].id = 1;
	fcgi_h[0].rd_size = sizeof(fcgi_h[0].read_buf);
	return &fcgi_h[0];
}

void fastcgi_disconnect(void *fcgi)
{
	struct fcgi_handle * _fcgi = (struct fcgi_handle*)fcgi;

	if (!fcgi)
		return;

	if (_fcgi->fd > 0) {
		close(_fcgi->fd);
		_fcgi->fd = 0;
	}

	return;
}


int fastcgi_begin_request(void *fcgi , unsigned short role, unsigned char flags)
{
	struct fcgi_handle * _fcgi = (struct fcgi_handle*)fcgi;
	int ret = 0;

	if (!fcgi)
		return -1;

	if (fastcgi_make_head(fcgi, &_fcgi->begin.head, fcgi_begin_request, sizeof(_fcgi->begin.begin_body)) < 0)
		return -1;

	_fcgi->begin.begin_body.role_h1 = (role >> 8) & 0xff;
	_fcgi->begin.begin_body.role_h0 = role & 0xff;
	_fcgi->begin.begin_body.flags = flags;

	ret = write (_fcgi->fd, &_fcgi->begin, sizeof(_fcgi->begin));
	if (ret < 0)
		return -1;
	
	return 0;
}

int fastcgi_params(void *fcgi, char *name, char *value)
{
	unsigned int name_len = 0;
	unsigned int value_len = 0;
	struct fcgi_handle * _fcgi = (struct fcgi_handle*)fcgi;
	int ret = 0;
	int last = 0;
	unsigned short acutal_len = 0;
	unsigned short content_len = 0;

	if (!fcgi || !name || !value)
		return -1;

	name_len = strlen(name);
	value_len = strlen(value);

	memset(_fcgi->params.params_body.data, 0, sizeof(_fcgi->params.params_body.data));

	if (name_len > HTTP_KEY_LEN || value_len > HTTP_PAREAMS_LEN)
		return -1;

	if (name_len <= 127)
		_fcgi->params.params_body.data[content_len++] = name_len;
	else {
		_fcgi->params.params_body.data[content_len++] = (name_len>>24 & 0x7f) | 0x80;
		_fcgi->params.params_body.data[content_len++] = name_len>>16 & 0xff;
		_fcgi->params.params_body.data[content_len++] = name_len>>8 & 0xff;
		_fcgi->params.params_body.data[content_len++] = name_len & 0xff;
	}

	if (value_len <= 127)
		_fcgi->params.params_body.data[content_len++] = value_len;
	else {
		_fcgi->params.params_body.data[content_len++] = (value_len>>24 & 0x7f) | 0x80;
		_fcgi->params.params_body.data[content_len++] = value_len>>16 & 0xff;
		_fcgi->params.params_body.data[content_len++] = value_len>>8 & 0xff;
		_fcgi->params.params_body.data[content_len++] = value_len & 0xff;
	}

	last = 0; /*(name_len+value_len+content_len)%8*/;
	acutal_len = last?name_len+value_len+content_len+8-last:name_len+value_len+content_len;

	if (fastcgi_make_head(fcgi, &_fcgi->params.head, fcgi_params, acutal_len) < 0)
		return -1;
	
	memcpy(&_fcgi->params.params_body.data[content_len], name, name_len);
	content_len += name_len;
	memcpy(&_fcgi->params.params_body.data[content_len], value, value_len);
	content_len += value_len;
	
	ret = write (_fcgi->fd, &_fcgi->params, sizeof(_fcgi->params.head)+acutal_len);
	if (ret < 0) {
		printf("write failed\n");
		return -1;
	}
	
	return 0;
	
}

int fastcgi_data(void *fcgi, unsigned char *content, unsigned short length)
{
	struct fcgi_handle * _fcgi = (struct fcgi_handle*)fcgi;
	struct fast_header head;
	int ret = 0;
	int acutal_len = 0;
	int last = 0;

	if (!_fcgi || !content)
		goto out;

	last = 0;/*length % 8 */;
	acutal_len = last?length+8-last:length;

	memset(_fcgi->content, 0, sizeof(_fcgi->content));
	if (fastcgi_make_head(_fcgi, &head, fcgi_stdin, acutal_len) < 0)
		goto out;

	if (sizeof(head) > sizeof(_fcgi->content))
		goto out;
	
	memcpy(_fcgi->content, &head, sizeof(head));
	if (length > sizeof(_fcgi->content)- sizeof(head))
		goto out;
	
	memcpy(_fcgi->content+sizeof(head), content, length);

	if (sizeof(head)+acutal_len > sizeof(_fcgi->content))
		goto out;
	
	ret = write (_fcgi->fd, _fcgi->content, sizeof(head)+acutal_len);
	if (ret < 0)
		goto out;

	return 0;
	
out:
	return -1;
}

int fastcgi_end_request(void *fcgi, unsigned char type)
{
	struct fcgi_handle * _fcgi = (struct fcgi_handle*)fcgi;
	int ret = 0;
	struct fast_header head;
	fastcgi_make_head(fcgi, &head, type, 0);
	
	ret = write (_fcgi->fd, &head, sizeof(head));
	if (ret < 0)
		return -1;
	
	return 0;

}

void fastcgi_end_begin(void *fcgi)
{
	struct fcgi_handle * _fcgi = (struct fcgi_handle*)fcgi;

	if (!fcgi)
		return;

	_fcgi->id++;

	return;
}

int fastcgi_make_head(void *fcgi, struct fast_header *head, unsigned char type, unsigned short length)
{
	struct fcgi_handle * _fcgi = (struct fcgi_handle*)fcgi;

	if (!head || !_fcgi)
		return -1;
	
	memset (head, 0, sizeof(struct fast_header));
	head->version = 1;
	head->type = type;
	head->request_id_h1 = _fcgi->id >> 8 & 0xff;
	head->request_id_h0 = _fcgi->id & 0xff;
	head->content_length_h1 = length >> 8 & 0xff;
	head->content_length_h0 = length & 0xff;

	return 0;
}

int fastcgi_get_fd(void *fcgi)
{
	struct fcgi_handle * _fcgi = (struct fcgi_handle*)fcgi;
	if (!_fcgi)
		return -1;

	return _fcgi->fd;
}

unsigned char *fastcgi_get_read_buf(void *fcgi, unsigned short *size)
{
	struct fcgi_handle * _fcgi = (struct fcgi_handle*)fcgi;
	if (!_fcgi)
		return NULL;

	*size = _fcgi->rd_size;

	return _fcgi->read_buf;
}



