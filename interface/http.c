#include "http.h"
#include <string.h>
#include <ctype.h>

struct http_mtd_map hmetd_map[metd_MAX] = {
	[metd_OPTIONS] = {.name = "OPTIONS"},
	[metd_HEAD] = {.name = "HEAD"},
	[metd_GET] = {.name = "GET"},
	[metd_POST] = {.name = "POST"},
	[metd_PUT] = {.name = "PUT"},
	[metd_DELETE] = {.name = "DELETE"},
	[metd_TRACE] = {.name = "TRACE"},
};

struct http_ver_map hver_map[ver_MAX] = {
	[ver_HTTP1_1] = {.name = "HTTP/1.1"},
};

struct http_res_code_map code_res_map[code_max] = {
	[code_100_continue] = {.desc = "Continue"},
	[code_101_switch_protocol] = {.desc = "Switching Protocols"},

	//sucess
	[code_200_ok] = {.desc = "OK"},
	[code_201_create] = {.desc = "Created"},
	[code_202_accept] = {.desc = "Accepted"},
	[code_203_non_auth] = {.desc = "Non-Authoritative Information"},
	[code_204_no_content] = {.desc = "No Content"},
	[code_205_reset_content] = {.desc = "Reset Content"},
	[code_206_partial_content] = {.desc = "Partial Content"},

	//redirect
	
	[code_300_mul_choice] = {.desc = "Multiple Choices"},
	[code_301_move_perman] = {.desc = "Moved Permanently"},
	[code_302_found] = {.desc = "Found"},
	[code_303_see_other] = {.desc = " See Other"},
	[code_304_not_mod] = {.desc = "Not Modified"},
	[code_305_use_proxy] = {.desc = " Use Proxy"},
	[code_306_no_use] = {.desc = "No Use"},
	[code_307_temp_redirect] = {.desc = "Temporary Redirect"},

	//client error
	[code_400_bad_request] = {.desc = "Bad Request"},
	[code_401_unauth] = {.desc = "Unauthorized"},
	[code_402_payment_require] = {.desc = "Payment Required"},
	[code_403_forbid] = {.desc = "Forbidden"},
	[code_404_not_found] = {.desc = "Not Found"},
	[code_405_method] = {.desc = "Method Not Allowed"},
	[code_406_not_accept] = {.desc = "Not Acceptable"},
	[code_407_proxy_auth_required] = {.desc = "Proxy Authentication Required"},
	[code_408_request_timeout] = {.desc = "Request Timeout"},
	[code_409_conflict] = {.desc = "Conflict"},
	[code_410_gone] = {.desc = "Gone"},
	[code_411_length_required] = {.desc = "Length Required"},
	[code_412_precondition_failed] = {.desc = "Precondition Failed"},
	[code_413_request_entity_too_long] = {.desc = "Request Entity Too Large"},
	[code_414_request_uri_too_long] = {.desc = "Request URI Too Long"},
	[code_415_unsup_media_type] = {.desc = "Unsupported Media Type"},
	[code_416_request_range_not_statis] = {.desc = "	Requested Range Not Satisfiable"},
	[code_417_expect_failed] = {.desc = "Expectation Failed"},

	//server error
	[code_500_internal_error] = {.desc = "Internal Server Error"},
	[code_501_not_implement] = {.desc = "Not Implemented"},
	[code_502_bad_gateway] = {.desc = "Bad Gateway"},
	[code_503_service_unavaliable] = {.desc = "Service Unavailable"},
	[code_504_gateway_timeout] = {.desc = "Gateway Timeout"},
	[code_505_http_ver_not_support] = {.desc = "HTTP Version Not Supported"},
};

struct http_header_map header_map[header_max] = {
		//common
	[header_cache_control] = { .name = "cache-control"},
	[header_date] = { .name = "date"},
	[header_connection] = { .name = "connection"},
	//request
	[header_accept] = { .name = "accept"},
	[header_accept_charset] = { .name = "ccept-charset"},
	[header_accept_encoding] = { .name = "accept-encoding"},
	[header_accept_language] = { .name = "accept-language"},
	[header_authorization] = { .name = "authorization"},
	[header_host] = { .name = "host"},
	[header_user_agent] = { .name = "user-agent"},
	[header_cookie] = { .name = "cookie"},
	[header_refer] = { .name = "refer"},
	[header_upgrade_insecure_requests] = { .name = "upgrade-insecure-requests"},
	//response
	[header_location] = { .name = "location"},
	[header_server] = { .name = "server"},
	[header_www_authenticate] = { .name = "www-authenticate"},
	[header_set_cookie] = { .name = "set-cookie"},

	//entiry
	[header_content_encoding] = { .name = "content-encoding"},
	[header_content_type] = { .name = "content-type"},
	[header_content_language] = { .name = "content-language"},
	[header_content_length] = { .name = "content-length"},
	[header_last_modified] = { .name = "last-modified"},
	[header_expires] = { .name = "expires"},
	[header_transfer_encoding] = { .name = "transfer-encoding"},

};

char *get_http_metd_name(unsigned int method)
{
	if (method >= metd_MAX || !method)
		return "";

	return hmetd_map[method].name;

}

unsigned int get_http_metd_id(char *name)
{
	unsigned int i = 0;
	if (!name)
		return 0;
	
	for(i = 0; i < metd_MAX;i++) {
		if (hmetd_map[i].name && !strcmp(name, hmetd_map[i].name))
			return i;
	}

	return 0;
}

unsigned int get_http_ver_id(char *name)
{
	unsigned int i = 0;
	if (!name)
		return 0;
	
	for(i = 0; i < ver_MAX;i++) {
		if (hver_map[i].name && !strcmp(name, hver_map[i].name))
			return i;
	}

	return 0;
}

char *get_http_ver_name(unsigned int ver)
{
	if (ver >= ver_MAX || !ver)
		return "";

	return hver_map[ver].name;
}

char *get_http_res_code_desc(unsigned int code)
{
	if (code >= code_max || !code)
		return "";

	return code_res_map[code].desc;
}


char *get_http_header_name(unsigned int header)
{
	if (header >= header_max || !header)
		return "";

	return header_map[header].name;
}

unsigned int get_http_header_id(char *name)
{
	unsigned int i = 0;
	if (!name)
		return 0;
	
	for(i = 0; i < header_max;i++) {
		if (header_map[i].name && !strcmp(name, header_map[i].name))
			return i;
	}

	return 0;


}

int hex2dec(char c)
{
	if ('0' <= c && c <= '9')
		return c - '0';
	else if ('a' <= c && c <= 'f')
		return c - 'a' + 10;
	else if ('A' <= c && c <= 'F')
		return c - 'A' + 10;

	return -1;
}

char dec2hex(unsigned short c)
{
	if (c <= 9)
		return c + '0';
	else if (10 <= c && c <= 15)
		return c + 'A' - 10;

	return -1;
}

int urlencode(char *url, int size, char *enurl, int en_size)
{
	int i = 0;
	int len = size;
	int res_len = 0;
	int i1, i0;
	int j;
	if (!url || !size || !en_size)
		return -1;

	for (i = 0; i < len && res_len < en_size; ++i)
	{
		char c = url[i];
		if (('0' <= c && c <= '9') ||
			('a' <= c && c <= 'z') ||
			('A' <= c && c <= 'Z') ||
			c == '/' || c == '.')
				enurl[res_len++] = c;
		else {
			j = (short int)c;
			if (j < 0)
				j += 256;

			i1 = j / 16;
			i0 = j - i1 * 16;
			enurl[res_len++] = '%';
			enurl[res_len++] = dec2hex(i1);
			enurl[res_len++] = dec2hex(i0);
		}
	}

	enurl[res_len] = '\0';
	return 0;
}

int  urldecode(char *en_url, int size, char*deurl ,int de_size)
{
	int i = 0;
	int len = size;
	int res_len = 0;
	char c1;
	char c;
	char c0;
	int num = 0;
	if (!en_url || !size || !de_size)
		return -1;
	
	for (i = 0; i < len && res_len < de_size; ++i)
	{
		c = en_url[i];
		if (c != '%')
			deurl[res_len++] = c;
		else {
			c1 = en_url[++i];
			c0 = en_url[++i];
			num = hex2dec(c1) * 16 + hex2dec(c0);
			deurl[res_len++] = num;
		}
	}
	
	deurl[res_len] = '\0';

	return 0;
}


void str_toupper(char *str, int size)
{
	int i = 0;
	if (!str || !size)
		return;

	for(i = 0; i < size; i++)
		str[i] = toupper(str[i]);

	return;
}

void str_tolower(char *str, int size)
{
	int i = 0;
	if (!str || !size)
		return;

	for(i = 0; i < size; i++)
		str[i] = tolower(str[i]);

	return;


}


