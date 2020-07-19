#include "ophttps.h"
#include "opweb_pub.h"
#include "errno.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "openssl/err.h"
#include "interface/http.h"
#include "ophttp.h"
#include "fastcgi.h"
#include <fcntl.h>
#include "libubox/usock.h"

struct ssl_client *opweb_alloc_idle_ssl_client(void)
{
		struct opweb *web = get_h_opweb();
		struct ssl_client *client = NULL;
		pthread_rwlock_wrlock(&web->rwlock);
	
		if (list_empty(&web->idle_https.head)) {
			goto out;
		}
	
		client = list_first_entry(&web->idle_https.head, struct ssl_client, node);
		if (!client) {
			goto out;
		}
		
		list_move_tail(&client->node, &web->busy_https.head);
		web->idle_https.num--;
		web->busy_https.num++;
		client->enable = 1;
		pthread_rwlock_unlock(&web->rwlock);
		return client;
	
	out:
		
		pthread_rwlock_unlock(&web->rwlock);
		return NULL;


}

void opweb_https_client_free(struct ssl_client *client)
{
	struct opweb *web = get_h_opweb();

	if(!client)
		return;
	
	pthread_rwlock_wrlock(&web->rwlock);
	SSL_shutdown(client->ssl);
	SSL_free(client->ssl);
	client->thread->client_num--;
	close(client->client_fd);
	evtimer_del(client->alive_timer);
	pthread_rwlock_unlock(&web->rwlock);

	opweb_release_idle_ssl_client(client);

	return;

}

void opweb_release_idle_ssl_client(struct ssl_client *client)
{

	struct opweb *web = get_h_opweb();
	if (!client)
		return;

	pthread_rwlock_wrlock(&web->rwlock);
	list_move_tail(&client->node, &web->idle_https.head);
	web->busy_https.num--;
	web->idle_https.num++;
	client->enable = 0;
	pthread_rwlock_unlock(&web->rwlock);

	return;
}

void opweb_https_timer(int s, short what, void *arg)
{
	(void)s;
	(void)what;
	
	struct ssl_client *client = NULL;

	client= (struct ssl_client *)arg;
	if (!client)
		return;

	opweb_https_client_free(client);

	return;
}

void opweb_https_read(evutil_socket_t fd, short what, void *arg)
{
	(void)fd;
	(void)what;

	int read_size = 0;
	int response_code = 0;
	struct opweb *web = get_h_opweb();
	(void)web;
	
	opweb_log_debug("opweb_https read， event = %d\n", what);
	struct ssl_client *client = (struct ssl_client *)arg;
	if (!client) {
		opweb_log_warn("client pointer is null\n");
		return;
	}

	read_size = SSL_read(client->ssl, client->read_buf, sizeof(client->read_buf));
	if (read_size <= 0) {
		opweb_https_client_free(client);
		opweb_log_debug("opweb_https read size = %d\n",read_size);
		return;
	}

	if (!read_size) {
		opweb_log_debug("opweb_https read is 0\n");
		goto out;
	}
	opweb_log_debug("opweb_https read %s\n",client->read_buf);
	evtimer_del(client->alive_timer);
	
	if ((response_code = ophttp_handle_header(client, client->read_buf, read_size, &client->proto)) < 0)
		goto out;
	
	opweb_log_debug("request path:%s, param:%s\n", client->proto.uri_path, client->proto.uri_param);

	#if 0
	if (client->doc_type == doc_type_unkonw) {
		opweb_log_warn("unkonw path:%s\n", client->proto.uri_path);
		goto out;
	}

	#endif
	
	if (client->proto.metd == metd_GET && (client->doc_type == doc_type_html || client->doc_type == doc_type_other ||
			client->doc_type == doc_type_unkonw))
		ophttp_static_docment(client, client->proto.uri_path, client->write_buf, sizeof(client->write_buf));
	else
		opweb_cb_fastcgi(client,  client->write_buf, sizeof(client->write_buf));
	
out:
	event_add(client->alive_timer, &client->t);

	if (!client->proto.keep_alive)
		opweb_https_client_free(client);

	return;
}

void opweb_https_accept(evutil_socket_t fd, short what, void *arg)
{
	(void)fd;
	(void)what;
	(void)arg;

	int ret,ret1 = 0;
	opweb_log_debug("https accept begin\n");
	struct ssl_client *client = NULL;
	socklen_t len = 0;
	struct sockaddr_in addr;
	int client_fd = 0;
	struct opweb_thread *thread = NULL;

	
	struct opweb *web = get_h_opweb();

	memset(&addr, 0, sizeof(addr));

	client_fd = accept(fd, (struct sockaddr*)&addr, &len);
	if (client_fd < 0) {
		opweb_log_warn("https accept:%s\n", strerror(errno));
		goto out;
	}
	
	client = opweb_alloc_idle_ssl_client();
	if (!client)
		goto out;

	client->ssl = SSL_new(web->openssl_ctx);
	if (!client->ssl) {
		opweb_log_warn("client[%d] SSL_new failed\n");
		goto out;
	}

	if (SSL_set_fd(client->ssl, client_fd) != 1) {
		opweb_log_warn("client[%d] SSL_set_fd failed\n");
		goto out;
	}

	if ((ret1 = SSL_accept(client->ssl)) != 1) {
		ret = SSL_get_error(client->ssl, ret1);
		ERR_print_errors_fp(fdopen(get_log_fd(web->log), "w"));
		opweb_log_warn("client[%d] SSL_accept failed,error = %d\n", ret);
		goto out;
	}

#if 0

	client->cert = SSL_get_peer_certificate(client->ssl);
	if (!client->cert) {
		opweb_log_warn("get peer client failed\n");
		goto out;
	}
#endif

	thread = opweb_get_thread();
	if (!thread) {
		opweb_log_warn("get thread failed\n");
		goto out;
	}

	client->ev_read = event_new(thread->base, client_fd, EV_PERSIST|EV_READ, opweb_https_read, client);
	if (!client->ev_read) {
		opweb_log_warn("client[%d] event_new failed\n");
		goto out;
	}

	if (event_add(client->ev_read, NULL)) {
		opweb_log_warn("client[%d] event_add failed\n");
		goto out;
	}
	client->alive_timer = evtimer_new(thread->base, opweb_https_timer, client);
	if (!client->alive_timer) {
		opweb_log_warn("client [%u] alloc evtimer_new failed\n", client_fd);
		goto out;
	}

	client->t.tv_sec = 600;
	if (event_add(client->alive_timer, &client->t) < 0) {
		
		opweb_log_warn("client [%u] add evtimer_new failed\n", client_fd);
		goto out;
	}
	
	pthread_rwlock_wrlock(&web->rwlock);
	client->thread = thread;
	thread->client_num++;
	memcpy(&client->addr, &addr, sizeof(client->addr));
	client->client_fd = client_fd;
	client->res_head_index = 1;
	pthread_rwlock_unlock(&web->rwlock);

	return;
out:
	if (client_fd > 0)
		close(client_fd);
	if (client)
		opweb_release_idle_ssl_client(client);
	
	return;
}

int opweb_cb_fastcgi(struct ssl_client *client, unsigned char *write_buf, unsigned int write_size)
{
	if(!client)
		goto out;

	(void)write_buf;
	(void)write_size;
	char buf_trunk[20];
	int i = 0,j = 0;
	int ret = 0;
	unsigned short size = 0;
	unsigned char * rd_buf = NULL;
	struct fast_header *header = NULL;
	unsigned short header_size  = 0;
	char key[HTTP_KEY_LEN];
	char value[HTTP_PAREAMS_LEN];
	char *str = NULL;
	char *start_str = NULL;
	int len = 0;
	char *end_str = NULL;
	int begin = 0,end = 0;
	int copy_len = 0;
	int index = 0;
	unsigned int content_len = 0;
	(void)content_len;

	
	void * fcgi = fastcgi_connect("127.0.0.1", 9000);
	if (!fcgi) {
		opweb_log_warn("connect fcgi failed\n");
		goto out;
	}

	fastcgi_begin_request(fcgi, fcgi_responser, fcgi_keep_conn);

	fastcgi_params(fcgi, "SCRIPT_FILENAME", client->proto.uri_path);
	fastcgi_params(fcgi, "REQUEST_METHOD", get_http_metd_name(client->proto.metd));
	opweb_log_debug("fast cgi:script_filename:%s, method:%s\n", client->proto.uri_path, get_http_metd_name(client->proto.metd));

	fastcgi_end_request(fcgi, fcgi_params);

	fastcgi_end_request(fcgi, fcgi_begin_request);
	
	fastcgi_end_begin(fcgi);

	ret = usock_wait_ready(fastcgi_get_fd(fcgi), 200);
	if (ret <= 0)
		goto out;

	rd_buf = fastcgi_get_read_buf(fcgi, &size);
	if (!rd_buf)
		goto out;

	index = 0;
	
read_again:
	while((ret = read(fastcgi_get_fd(fcgi), rd_buf, sizeof(struct fast_header))) > 0) {
	
		if (ret < (int)sizeof(struct fast_header)) {
			opweb_log_warn("get header size[%d] not euql\n", ret);
			goto out;
		}

		header = (struct fast_header*)rd_buf;
		if (header->type != fcgi_stdout) {
			header_size = header->content_length_h1 << 8 | header->content_length_h0;
			header_size = header_size > size?size:header_size;
			opweb_log_debug("get header type[%hhu] ignore, try read[%hu] skip it\n", header->type, header_size);
			ret = read(fastcgi_get_fd(fcgi), rd_buf, header_size);
			if (ret <= 0 && header_size) {
				opweb_log_warn("get header type[%hhu] ignore, try read[%hu] skip it, read failed\n", header->type, header_size);
				goto out;
			}
			goto read_again;
		}

		opweb_log_warn("fast cgi type = %d\n", header->type);
		header_size = header->content_length_h1 << 8 | header->content_length_h0;
		header_size = header_size > size?size:header_size;
		ret = read(fastcgi_get_fd(fcgi), rd_buf, header_size);
		if (ret <= 0) {
			opweb_log_warn("stdout :try read[%hu] failed\n", header_size);
			goto out;
		}

		if (index) {
			content_len += header_size;
			goto continue_content;
		}
	
		start_str = (char*)rd_buf;
		end_str = strstr((char*)rd_buf, "\r\n\r\n");
		if (!end_str) {
			opweb_log_warn("stdout :can not found %s\n","\r\n\r\n");
			goto out;
		}
		
		content_len = header_size-(end_str-start_str) - 4;


		while((str = strstr(start_str, "\r\n")) && (str < end_str+2)) {
			len = str - start_str;
			begin = 0;

			for (i = 0; i < len; i++) {
				if (start_str[i] == ':') {
					end = i;
					copy_len = end-begin >= (int)sizeof(key)?(int)sizeof(key)-1:end-begin;
					if (copy_len <= 0)
						goto out;
					memset(key,0,sizeof(key));
					memcpy(key,start_str+begin, copy_len);
					str_tolower(key, copy_len);
					
					for (j = i+1; j < len;j++) {
						if (start_str[j] == ' ')
							continue;
						break;
					}
				
					if (j >= len)
						goto out;

					begin = j;
					end = len;
					copy_len = end-begin >= (int)sizeof(value)?(int)sizeof(value)-1:end-begin;
					if (copy_len <= 0)
						goto out;
				
					memset(value,0,sizeof(value));
					memcpy(value,start_str+begin, copy_len);
					str_tolower(value, copy_len);
					if (!strstr(value, "php") && !strstr(value, "PHP"))
						ophttp_print(client, "%s: %s\r\n", key, value);
					break;
				}

			}

			start_str = str+2;
		}

		if (client->proto.keep_alive)
				ophttp_print(client, "%s: %s\r\n", get_http_header_name(header_connection), "keep-alive");
		else
				ophttp_print(client, "%s: %s\r\n", get_http_header_name(header_connection), "close");


		ophttp_print_first(client,"%s 200 %s\r\n", get_http_ver_name(client->proto.ver), get_http_res_code_desc(code_200_ok));
		ophttp_print(client, "%s: %s\r\n", get_http_header_name(header_transfer_encoding), "chunked");
		
		//ophttp_print(client,"%s: %u\r\n", get_http_header_name(header_content_length), content_len);

		ophttp_print_end(client, "\r\n");

		ophttp_resonse(client);

		snprintf (buf_trunk, sizeof(buf_trunk), "%x\r\n", content_len);
		SSL_write(client->ssl , buf_trunk, strlen(buf_trunk));
		SSL_write(client->ssl , end_str+4, content_len);
		SSL_write(client->ssl , "\r\n", 2);
		index++;
		goto read_again;

continue_content:
		snprintf (buf_trunk, sizeof(buf_trunk), "%x\r\n", ret);
		SSL_write(client->ssl , buf_trunk, strlen(buf_trunk));
		SSL_write(client->ssl , rd_buf, ret);
		SSL_write(client->ssl , "\r\n", 2);
	}
	
	SSL_write(client->ssl , "0\r\n\r\n", 5);
	//opweb_log_debug("stdout content_size :%u\n", content_len);
	fastcgi_disconnect(fcgi);

	return 0;
out:
	SSL_write(client->ssl , "0\r\n\r\n", 5);
	fastcgi_disconnect(fcgi);
	return -1;
}


