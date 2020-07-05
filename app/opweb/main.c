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
#include <dirent.h>

#define OPWEB_CONF_PATH "/home/isir/developer/openserver/app/opweb/opweb_conf.xml"

int main(int argc, char**argv)
{
	(void)argc;
	(void)argv;
	int i = 0;
	DIR *dir = NULL;
	struct dirent *file = NULL;
	(void)file;
	(void)dir;
	
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();
	SSL_load_error_strings();

	struct opweb *web = NULL; 
	web = opweb_init();
	if (!web)
		goto out;

	web->log = log_init(MODULE_OPWEB);
	if (!web->log)
		goto out;

	opweb_log_debug("opweb init\n");
	

	web->openssl_ctx =  SSL_CTX_new(TLS_server_method());
	if (!web->openssl_ctx) {
		opweb_log_error("openssl ctx create failed\n");
		goto out;
	}

	if (opweb_config_parse(OPWEB_CONF_PATH) < 0) {
		opweb_log_error("opweb init config failed\n");
		goto out;
	}

#if 1
	//SSL_CTX_set_verify(web->openssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

	dir = opendir(web->ca_trust_path);
	if (!dir)
		goto out;
	while((file = readdir(dir))) {
		if (file->d_type == DT_REG) {
			snprintf((char*)web->bus_webbuf, sizeof(web->bus_webbuf), "%s/%s", web->ca_trust_path, file->d_name);
			opweb_log_debug("trust ca:%s\n", (char*)web->bus_webbuf);
			if (SSL_CTX_load_verify_locations(web->openssl_ctx, (char*)web->bus_webbuf, NULL) != 1) {
				opweb_log_error("ssl load trust cert [%s] failed\n", (char*)web->bus_webbuf);
				goto out;
			}
		}
	}
	closedir(dir);
#endif

	if(SSL_CTX_use_certificate_file(web->openssl_ctx, web->pub_path ,SSL_FILETYPE_PEM) != 1) {
		opweb_log_error("SSL_CTX_use_certificate_file failed\n");
		goto out;
	}
	if(SSL_CTX_use_PrivateKey_file(web->openssl_ctx, web->priv_path ,SSL_FILETYPE_PEM) != 1) {
		opweb_log_error("SSL_CTX_use_PrivateKey_file failed\n");
		goto out;
	}
	
	if (SSL_CTX_check_private_key(web->openssl_ctx) != 1) {
		opweb_log_error("SSL_CTX_check_private_key failed\n");
		goto out;
	}

	SSL_CTX_set_options(web->openssl_ctx, SSL_OP_NO_SSLv2);

	web->base = event_base_new();
	if (!web->base)
		goto out;

	web->bus = bus_connect(MODULE_OPWEB, opweb_bus_cb, opweb_bus_disconnect, web);
	if (!web->bus)
		goto out;

	web->http_fd = usock(USOCK_TCP|USOCK_SERVER, "0.0.0.0", usock_port(web->hp_conf.port));
	if (web->http_fd < 0)
		goto out;

	web->https_fd = usock(USOCK_TCP|USOCK_SERVER, "0.0.0.0", usock_port(web->hps_conf.port));
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
