#include <stdio.h>
#include "opweb.h"
#include "interface/module.h"
#include "interface/log.h"
#include "libubox/utils.h"
#include "interface/bus.h"
#include "libubox/usock.h"
#include "event.h"
#include "ophttps.h"
#include "openssl/ssl.h"
#include "openssl/types.h"

int main(int argc, char**argv)
{
	(void)argc;
	(void)argv;
	int i = 0;

	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	struct opweb *web = NULL; 
	web = opweb_init();
	if (!web)
		goto out;

	web->log = log_init(MODULE_OPWEB);
	if (!web->log)
		goto out;

	opweb_log_debug("opweb init\n");

	/*openssl req -newkey rsa:4096 -nodes -sha256 -keyout ca.key -x509 -days 365 -out ca.crt */
	web->openssl_ctx =  SSL_CTX_new(SSLv23_server_method());
	if (!web->openssl_ctx) {
		opweb_log_error("openssl ctx create failed\n");
		goto out;
	}


	if(SSL_CTX_use_certificate_file(web->openssl_ctx, "/home/isir/developer/openserver/app/opweb/conf/ca.crt",SSL_FILETYPE_PEM) != 1) {
		opweb_log_error("SSL_CTX_use_certificate_file failed\n");
		goto out;
	}
	if(SSL_CTX_use_PrivateKey_file(web->openssl_ctx, "/home/isir/developer/openserver/app/opweb/conf/ca.key",SSL_FILETYPE_PEM) != 1) {
		opweb_log_error("SSL_CTX_use_PrivateKey_file failed\n");
		goto out;
	}
	if (SSL_CTX_check_private_key(web->openssl_ctx) != 1) {
		opweb_log_error("SSL_CTX_check_private_key failed\n");
		goto out;
	}

	web->base = event_base_new();
	if (!web->base)
		goto out;

	web->bus = bus_connect(MODULE_OPWEB, opweb_bus_cb, opweb_bus_disconnect, web);
	if (!web->bus)
		goto out;

	web->http_fd = usock(USOCK_TCP|USOCK_SERVER, "0.0.0.0", usock_port(60000));
	if (web->http_fd < 0)
		goto out;

	web->https_fd = usock(USOCK_TCP|USOCK_SERVER, "0.0.0.0", usock_port(60001));
	if (web->https_fd < 0)
		goto out;

	for (i = 0; i < MAX_OPWEB_THREAT_NUM; i++) {
		web->thread[i].base = event_base_new();
		web->thread[i].index = i;
		if (!web->thread[i].base)
			goto out;

		if (pthread_create(&web->thread[i].thread_id, NULL, opweb_thread_job, web->thread[i].base))
			goto out;
	}

	web->ehttp = event_new(web->base, web->http_fd, EV_PERSIST|EV_READ, opweb_http_accept, web);
	if (event_add(web->ehttp, NULL))
		goto out;

	web->ehttps = event_new(web->base, web->https_fd, EV_PERSIST|EV_READ, opweb_https_accept, web);
	if (event_add(web->ehttps, NULL))
		goto out;

	event_base_dispatch(web->base);

	return 0;
out:
	opweb_exit(web);
	return 0;
}
