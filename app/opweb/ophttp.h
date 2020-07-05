#ifndef _OPHTTP_H__
#define _OPHTTP_H__
#include "interface/http.h"
#include "opweb.h"

#define HTTP_METHOD_SIZE 64
#define HTTP_VER_SIZE 64
enum {
	doc_type_unkonw = 0,
	doc_type_html,
	doc_type_php,
	doc_type_other,
	doc_type_max,
};

int ophttp_handle_header(struct ssl_client *client,unsigned char* request, unsigned int size, struct http_proto *proto);
void ophttp_static_docment(struct ssl_client *client, const char *path, unsigned char *write_buf, unsigned int write_size);

#endif

