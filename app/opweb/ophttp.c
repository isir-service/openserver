#include "ophttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface/http.h"
#include "opweb.h"

enum http_rescode ophttp_get_header(unsigned char* request, unsigned int size, struct http_proto *proto)
{
	char *start_str = NULL;
	char *end_str = NULL;
	char * str = NULL;
	char buf[1024] = {};
	int begin = 0;
	int end = 0;
	int len = 0;
	int copy_len = 0;
	unsigned int num = 0;
	int i = 0,j = 0;;
	
	if (!request || !size || !proto)
		return code_400_bad_request;

	start_str = (char*)request;

	end_str = strstr((char*)request, "\r\n\r\n");
	if (!end_str)
		return code_400_bad_request;
	
	memset(proto, 0 ,sizeof(struct http_proto));

	begin = 0;
	num = 0;

	while((str = strstr((char*)start_str, "\r\n"))) {
		if (!num) {/*common header*/
			len = str- start_str;
			begin = 0;
			for(i = 0; i < len;i++) {
				if (start_str[i] == ' ' || (i == len-1)) { /*http method*/
					if (i == len -1)
						end = len;
					else 
						end = i;
					copy_len = end-begin >= (int)sizeof(buf)?(int)sizeof(buf)-1:end-begin;
					if (copy_len <= 0)
						return code_405_method;

					memset(buf,0,sizeof(buf));
					memcpy(buf,start_str+begin, copy_len);
					if (!proto->metd) {
						proto->metd = get_http_metd_id(buf);
						if (!proto->metd)
							return code_405_method;
					} else if (!proto->uri || !strlen(proto->uri)) {
						if (urldecode(buf, copy_len, proto->uri, sizeof(proto->uri)-1) < 0)
							return code_400_bad_request;
					} else if (!proto->ver) {
						proto->ver = get_http_ver_id(buf);
						if (!proto->ver)
							return code_400_bad_request;

						break;
					}
						
					begin = i+1;
				}
			}
			
		}else  {
			if (!proto->metd || !proto->ver || !proto->uri || !strlen(proto->uri))
				return code_400_bad_request;

			len = str- start_str;
			begin = 0;
			for(i = 0; i < len;i++) {
				if (start_str[i] == ':') {
					end = i;
					copy_len = end-begin >= (int)sizeof(buf)?(int)sizeof(buf)-1:end-begin;
					if (copy_len <= 0)
						return code_405_method;
					memset(buf,0,sizeof(buf));
					memcpy(buf,start_str+begin, copy_len);
					str_tolower(buf, copy_len);
					opweb_log_debug("request header key:<%s>\n",buf);
					if(!get_http_header_id(buf))
						opweb_log_warn("server unkown this header:%s\n",buf);
					for (j = i+1; j < len;j++) {
						if (start_str[j] == ' ')
							continue;
						break;
					}

					if (j >= len) {
						opweb_log_warn("this header<%s>: value is none, drop it\n", buf);
						return code_400_bad_request;
					}
					
					begin = j;
					end = len;
					copy_len = end-begin >= (int)sizeof(buf)?(int)sizeof(buf)-1:end-begin;
					if (copy_len <= 0)
						return code_405_method;

					memset(buf,0,sizeof(buf));
					memcpy(buf,start_str+begin, copy_len);
					str_tolower(buf, copy_len);
					
					opweb_log_debug("request header value:<%s>\n",buf);
					break;
					
				}
			}
		}
		
		start_str = str+2;
		num++;
	}


	
	
	return 0;

}

