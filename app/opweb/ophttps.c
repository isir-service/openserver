#include "ophttps.h"
#include "opweb.h"
#include "errno.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "openssl/err.h"
#include "interface/http.h"

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
	opweb_log_debug("opweb_https read， event = %d\n", what);
	struct ssl_client *client = (struct ssl_client *)arg;
	if (!client) {
		opweb_log_warn("client pointer is null\n");
		return;
	}

	read_size = SSL_read(client->ssl, client->read_buf, sizeof(client->read_buf));
	if (read_size < 0) {
		opweb_https_client_free(client);
		opweb_log_debug("opweb_https read size = %d\n",read_size);
		return;
	}

	if (!read_size) {
		opweb_log_debug("opweb_https read is 0\n");
		goto out;
	}

	evtimer_del(client->alive_timer);
	
	if (!ophttp_handle_header(client->read_buf, read_size, &client->proto))
		goto out;
	
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

	
	if (SSL_accept(client->ssl) != 1) {
		ret = SSL_get_error(client->ssl, ret1);
		//ERR_print_errors_fp(stderr);
		opweb_log_warn("client[%d] SSL_accept failed,error = %d\n", ret);
		goto out;
	}

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

	client->t.tv_sec = 120;
	if (event_add(client->alive_timer, &client->t) < 0) {
		
		opweb_log_warn("client [%u] add evtimer_new failed\n", client_fd);
		goto out;
	}
	
	pthread_rwlock_wrlock(&web->rwlock);
	client->thread = thread;
	thread->client_num++;
	memcpy(&client->addr, &addr, sizeof(client->addr));
	client->client_fd = client_fd;
	pthread_rwlock_unlock(&web->rwlock);

	return;
out:
	if (client_fd > 0)
		close(client_fd);
	if (client)
		opweb_release_idle_ssl_client(client);
	
	return;
}

