#ifndef __FAST_CGI_H__
#define __FAST_CGI_H__
#include "ophttp.h"
#include "interface/http.h"

struct fast_header {
	unsigned char version;
	unsigned char type;
	unsigned char request_id_h1;
	unsigned char request_id_h0;
	unsigned char content_length_h1;
	unsigned char content_length_h0;
	unsigned char padding_length;
	unsigned char reserved;
}__attribute__ ((aligned (1)));

enum fcgi_type {
	fcgi_begin_request     = 1, /* (WEB->FastCGI) */
	fcgi_abort_request     = 2, /* (WEB->FastCGI) */
	fcgi_end_request       = 3, /* (FastCGI->WEB) */
	fcgi_params            = 4, /* (WEB->FastCGI) */
	fcgi_stdin             = 5, /* (WEB->FastCGI) */
	fcgi_stdout            = 6, /* (FastCGI->WEB) */
	fcgi_stderr            = 7, /* (FastCGI->WEB) */
	fcgi_data              = 8, /* (WEB->FastCGI) */
	fcgi_get_value         = 9, /* (WEB->FastCGI) */
	fcgi_get_values_result = 10, /* (FastCGI->WEB) */
	fcgi_unknown_type      = 11,
};

struct fcgi_begin_request_body{
	unsigned char role_h1;
	unsigned char role_h0;
	unsigned char flags;
	unsigned char reserved[5];
}__attribute__ ((aligned (1)));

enum fcgi_role {
	fcgi_responser  = 1, /* response */
	fcgi_authorizer = 2, /* auth */
	fcgi_filter     = 3, /* filter */
};

enum fcgi_flags {
	fcgi_keep_conn = 1,
};

struct fcgi_end_request_body {
	unsigned char app_status_b3;
	unsigned char app_status_b2;
	unsigned char app_status_b1;
	unsigned char app_status_b0;
	unsigned char protocol_status;
	unsigned char reserved[3];
}__attribute__ ((aligned (1)));

enum fcgi_protocol_status {
	fcgi_request_cpmplete = 0,/* complete */
	fcgi_cant_mpx_conn    = 1, /* nort support */
	fcgi_over_load        = 2, /* overload */
	fcgi_unknown_role     = 3, /* unknown role */
};

struct fcgi_params_body {
	
	unsigned char data[8+HTTP_KEY_LEN+HTTP_PAREAMS_LEN];
	#if 0
	unsigned char name_length_h3;
	unsigned char name_length_h2;
	unsigned char name_length_h1;
	unsigned char name_length_h0;
	unsigned char value_length_h3;
	unsigned char value_length_h2;
	unsigned char value_length_h1;
	unsigned char value_length_h0;
	unsigned char name_data[HTTP_KEY_LEN];
	unsigned char value_data[HTTP_PAREAMS_LEN];
	#endif
}__attribute__ ((aligned (1)));

void *fastcgi_connect(char *server, unsigned short port);

void fastcgi_disconnect(void *fcgi);

int fastcgi_begin_request(void *fcgi , unsigned short role, unsigned char flags);

int fastcgi_params(void *fcgi, char *name, char *value);

int fastcgi_data(void *fcgi, unsigned char *content, unsigned short length);

int fastcgi_end_request(void *fcgi, unsigned char type);

void fastcgi_end_begin(void *fcgi);

int fastcgi_make_head(void *fcgi, struct fast_header *head, unsigned char type, unsigned short length);

int fastcgi_get_fd(void *fcgi);

unsigned char *fastcgi_get_read_buf(void *fcgi, unsigned short *size);


#endif
