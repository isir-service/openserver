#include "ophttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface/http.h"
#include "opweb.h"
#include "libubox/utils.h"
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int ophttp_handle_header(struct ssl_client *client,unsigned char* request, unsigned int size, struct http_proto *proto)
{
	char *start_str = NULL;
	char *end_str = NULL;
	char * str = NULL;
	char *str_tmp = NULL;
	char buf[HTTP_PAREAMS_LEN] = {};
	char key[HTTP_KEY_LEN];
	char value[HTTP_PAREAMS_LEN];
	int begin = 0;
	int end = 0;
	int len = 0;
	int copy_len = 0;
	unsigned int num = 0;
	int i = 0,j = 0;
	unsigned int header_id = 0;
	struct opweb *web = get_h_opweb();

	
	if (!request || !size || !proto)
		return -code_400_bad_request;

	start_str = (char*)request;

	end_str = strstr((char*)request, "\r\n\r\n");
	if (!end_str)
		return -code_400_bad_request;
	
	memset(proto, 0 ,sizeof(struct http_proto));

	begin = 0;
	num = 0;

	while((str = strstr((char*)start_str, "\r\n")) && (str < end_str+2)) {
		len = str- start_str;
		begin = 0;
		if (!num) {/*common header*/

			for(i = 0; i < len;i++) {
				if (start_str[i] == ' ' || (i == len-1)) { /*http method*/
					if (i == len -1)
						end = len;
					else 
						end = i;
					copy_len = end-begin >= (int)sizeof(buf)?(int)sizeof(buf)-1:end-begin;
					if (copy_len <= 0)
						return -code_405_method;

					memset(buf,0,sizeof(buf));
					memcpy(buf,start_str+begin, copy_len);
					if (!proto->metd) {
						proto->metd = get_http_metd_id(buf);
						if (!proto->metd)
							return -code_405_method;
					} else if (!proto->uri || !strlen(proto->uri)) {
						if (urldecode(buf, copy_len, proto->uri, sizeof(proto->uri)-1) < 0)
							return -code_400_bad_request;
					} else if (!proto->ver) {
						proto->ver = get_http_ver_id(buf);
						if (!proto->ver)
							return -code_400_bad_request;

						break;
					}
						
					begin = i+1;
				}
			}

			memset(proto->uri_path, 0, sizeof(proto->uri_path));
			copy_len = (int)strlen(web->www_root) >= (int)sizeof(proto->uri_path)?(int)sizeof(proto->uri_path)-1:(int)strlen(web->www_root);
			memcpy(proto->uri_path, web->www_root, copy_len);
			str_tmp = strstr(proto->uri, "?");
			if (str_tmp) {
				copy_len = str_tmp-proto->uri >= (int)sizeof(proto->uri_path)?(int)sizeof(proto->uri_path)-1:(str_tmp-proto->uri);
				memcpy(proto->uri_path+strlen(web->www_root), proto->uri, copy_len);
				strlcpy(proto->uri_param, str, sizeof(proto->uri_param));
			} else {
				if (!strcmp(proto->uri,"/")) {
					snprintf(proto->uri_path+strlen(web->www_root), sizeof(proto->uri_path)-strlen(web->www_root), "/index.html");
					if (!access(proto->uri_path,F_OK))
						client->doc_type = doc_type_html;
					else {
						snprintf(proto->uri_path+strlen(web->www_root), sizeof(proto->uri_path)-strlen(web->www_root), "/index.php");
						if (!access(proto->uri_path,F_OK))
							client->doc_type = doc_type_php;
						else {
							snprintf(proto->uri_path+strlen(web->www_root), sizeof(proto->uri_path)-strlen(web->www_root), "/");
							client->doc_type = doc_type_unkonw;
						}
					}
				} else {
					strlcpy(proto->uri_path+strlen(web->www_root), proto->uri, sizeof(proto->uri_path)-strlen(web->www_root));
					if (!access(proto->uri_path, F_OK)) {
						if (strstr(proto->uri_path, ".php"))
							client->doc_type = doc_type_php;
						else if (strstr(proto->uri_path, ".html"))
							client->doc_type = doc_type_html;
						else
							client->doc_type = doc_type_other;
					} else {
						snprintf(proto->uri_path+strlen(web->www_root), sizeof(proto->uri_path)-strlen(web->www_root), "%s/%s",proto->uri, ".html");
						if (!access(proto->uri_path, F_OK))
							client->doc_type = doc_type_html;
						else {
							snprintf(proto->uri_path+strlen(web->www_root), sizeof(proto->uri_path)-strlen(web->www_root), "%s/%s",proto->uri, ".php");
							if (!access(proto->uri_path, F_OK))
								client->doc_type = doc_type_php;
							else {
								strlcpy(proto->uri_path+strlen(web->www_root), proto->uri, sizeof(proto->uri_path)-strlen(web->www_root));
								client->doc_type = doc_type_unkonw;
							}
						}

					}
				}

				memset(proto->uri_param, 0, sizeof(proto->uri_param));
			}
			
		}else  {
			if (!proto->metd || !proto->ver || !proto->uri || !strlen(proto->uri))
				return -code_400_bad_request;

			for(i = 0; i < len;i++) {
				if (start_str[i] == ':') {
					end = i;
					copy_len = end-begin >= (int)sizeof(key)?(int)sizeof(key)-1:end-begin;
					if (copy_len <= 0)
						return -code_405_method;
					memset(key,0,sizeof(key));
					memcpy(key,start_str+begin, copy_len);
					str_tolower(key, copy_len);
					opweb_log_debug("request header key:<%s>\n",key);
					if(!(header_id = get_http_header_id(key)))
						opweb_log_warn("server unkown this header:%s\n",key);

					for (j = i+1; j < len;j++) {
						if (start_str[j] == ' ')
							continue;
						break;
					}

					if (j >= len) {
						opweb_log_warn("this header<%s>: value is none, drop it\n", key);
						return -code_400_bad_request;
					}
					
					begin = j;
					end = len;
					copy_len = end-begin >= (int)sizeof(value)?(int)sizeof(value)-1:end-begin;
					if (copy_len <= 0)
						return -code_405_method;

					memset(value,0,sizeof(value));
					memcpy(value,start_str+begin, copy_len);
					str_tolower(value, copy_len);
					handle_http_value(header_id, proto, value, copy_len);
					if (header_id) {
						strlcpy(proto->key[header_id].value, value, sizeof(proto->key[header_id].value));
						proto->key[header_id].valid = 1;
					}

					opweb_log_debug("request header value:<%s>\n",value);
					break;
					
				}
			}
		}
		
		start_str = str+2;
		num++;
	}

	return 0;

}

void ophttp_static_docment(struct ssl_client *client, const char *path, unsigned char *write_buf, unsigned int write_size)
{

	char * write_str = NULL;
	int ret = 0;
	time_t t = 0;
	int index = 0;
	t = time(NULL);
	int content_size = 0;
	(void)t;
	int fd = 0;
	struct stat st;
	
	write_str = (char*)write_buf;

	if (!client || !path || !write_size)
		goto out;

	index = 0;
	
	if (stat(path, &st) < 0) {

		ret = snprintf(write_str+index, write_size - index, "%s 404 %s\r\n", get_http_ver_name(client->proto.ver), get_http_res_code_desc(code_404_not_found));
		if (ret <= 0)
			goto out;
		
		index += ret;

		if (client->proto.keep_alive)
			ret = snprintf(write_str+index, write_size - index, "%s: %s\r\n", get_http_header_name(header_connection), "keep-alive");
		else
			ret = snprintf(write_str+index, write_size - index, "%s: %s\r\n", get_http_header_name(header_connection), "close");
		if (ret <= 0)
			goto out;

		index += ret;

		ret = snprintf(write_str+index, write_size - index, "%s: %d\r\n", get_http_header_name(header_content_length), 0);
		if (ret <= 0)
			goto out;

		index += ret;
		
		ret = snprintf(write_str+index, write_size - index, "\r\n");

		if (ret <= 0)
			goto out;
		index += ret;
	
		goto out;
	}

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		ret = snprintf(write_str+index, write_size - index, "%s 500 %s\r\n", get_http_ver_name(client->proto.ver), get_http_res_code_desc(code_500_internal_error));
		if (ret <= 0)
			goto out;
		
		index += ret;
		goto out;
	}

	content_size = (int)st.st_size;
	ret = snprintf(write_str+index, write_size - index, "%s 200 %s\r\n", get_http_ver_name(client->proto.ver), get_http_res_code_desc(code_200_ok));
	if (ret <= 0)
		goto out;
	
	index += ret;

	ret = snprintf(write_str+index, write_size - index, "%s: %d\r\n", get_http_header_name(header_content_length), content_size);
	if (ret <= 0)
		goto out;
	
	index += ret;

	if (client->proto.keep_alive)
		ret = snprintf(write_str+index, write_size - index, "%s: %s\r\n", get_http_header_name(header_connection), "keep-alive");
	else
		ret = snprintf(write_str+index, write_size - index, "%s: %s\r\n", get_http_header_name(header_connection), "close");
	if (ret <= 0)
		goto out;
	
	index += ret;


	ret = snprintf(write_str+index, write_size - index, "\r\n");
	if (ret <= 0)
		goto out;
	index += ret;

	ret = read(fd, write_str+index, write_size - index);
	close(fd);
	index+= ret;
	
out:
	opweb_log_debug("server response:%s,content<%d><%d>\n", write_str, content_size, ret);
	SSL_write(client->ssl, write_str, index);
	return;
}


