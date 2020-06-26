#ifndef _OPHTTP_H__
#define _OPHTTP_H__
#include "interface/http.h"

#define HTTP_METHOD_SIZE 64
#define HTTP_VER_SIZE 64

enum http_rescode ophttp_handle_header(unsigned char* request, unsigned int size, struct http_proto *proto);

#endif

