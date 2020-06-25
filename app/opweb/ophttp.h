#ifndef _OPHTTP_H__
#define _OPHTTP_H__
#include "interface/http.h"

#define HTTP_URI 1024
#define HTTP_HOST 128

#define HTTP_METHOD_SIZE 64
#define HTTP_VER_SIZE 64

struct http_proto {
	unsigned int metd ;
	unsigned int ver;
	char uri[HTTP_URI];
	char host[HTTP_HOST];

};


enum http_rescode ophttp_get_header(unsigned char* request, unsigned int size, struct http_proto *proto);

#endif

