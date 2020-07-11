#ifndef __HTTP_H__
#define __HTTP_H__

#define HTTP_URI 1024
#define HTTP_HOST 128

#define HTTP_KEY_LEN 96
#define HTTP_PAREAMS_LEN 496


struct http_mtd_map {
	char *name;
};

struct http_ver_map {
	char *name;
};

struct http_res_code_map {
	char *desc;
};

struct http_proto;

typedef void (*header_type_cb) (struct http_proto *, char *, int);
struct http_header_map {
	char *name;
	header_type_cb cb;
};

enum http_method {
	metd_OPTIONS = 1,
	metd_HEAD,
	metd_GET,
	metd_POST,
	metd_PUT,
	metd_DELETE,
	metd_TRACE,
	metd_MAX,
};

enum http_version {
	ver_HTTP1_1 = 1,
	ver_MAX,
};

enum http_rescode {
	//information
	code_100_continue = 1,
	code_101_switch_protocol,

	//sucess
	code_200_ok,
	code_201_create,
	code_202_accept,
	code_203_non_auth,
	code_204_no_content,
	code_205_reset_content,
	code_206_partial_content,

	//redirect
	code_300_mul_choice,
	code_301_move_perman,
	code_302_found,
	code_303_see_other,
	code_304_not_mod,
	code_305_use_proxy,
	code_306_no_use,
	code_307_temp_redirect,

	//client error
	code_400_bad_request,
	code_401_unauth,
	code_402_payment_require,
	code_403_forbid,
	code_404_not_found,
	code_405_method,
	code_406_not_accept,
	code_407_proxy_auth_required,
	code_408_request_timeout,
	code_409_conflict,
	code_410_gone,
	code_411_length_required,
	code_412_precondition_failed,
	code_413_request_entity_too_long,
	code_414_request_uri_too_long,
	code_415_unsup_media_type,
	code_416_request_range_not_statis,
	code_417_expect_failed,

	//server error
	code_500_internal_error,
	code_501_not_implement,
	code_502_bad_gateway,
	code_503_service_unavaliable,
	code_504_gateway_timeout,
	code_505_http_ver_not_support,

	code_max,
};

enum http_header {

	//common
	header_cache_control = 1,
	header_date,
	header_connection,

	//entiry
	header_content_encoding,
	header_content_type,
	header_content_language,
	header_content_length,
	header_last_modified,
	header_expires,
	header_transfer_encoding,

	//request
	header_accept,
	header_accept_charset,
	header_accept_encoding,
	header_accept_language,
	header_authorization,
	header_host,
	header_user_agent,
	header_cookie,
	header_refer,
	header_upgrade_insecure_requests,

	//response
	header_location,
	header_server,
	header_www_authenticate,
	header_set_cookie,

	header_max,

};

struct header_key {
	char value[HTTP_PAREAMS_LEN];
	int valid;
};

struct http_proto {
	unsigned int metd ;
	unsigned int ver;
	char uri[HTTP_URI];
	char host[HTTP_HOST];
	char uri_path[HTTP_URI];
	char uri_param[HTTP_URI];
	struct  header_key key[header_max];
	unsigned int keep_alive;
};

enum http_header_value {
	hd_value_keep_alive = 1,


};

char *get_http_metd_name(unsigned int method);
unsigned int get_http_metd_id(char *name);

char *get_http_ver_name(unsigned int ver);
unsigned int get_http_ver_id(char *name);

char *get_http_res_code_desc(unsigned int code);

char *get_http_header_name(unsigned int header);
unsigned int get_http_header_id(char *name);
void handle_http_value(unsigned int header, struct http_proto *proto, char *value, int size);

int urlencode(char *url, int size, char *enurl, int en_size);
int urldecode(char *en_url, int size, char*deurl ,int de_size);

void str_toupper(char *str, int size);
void str_tolower(char *str, int size);

void header_connection_cb(struct http_proto *proto, char *value, int size);

char *get_http_content_name_by_tail(char *tail);

#endif
