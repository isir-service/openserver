#ifndef __OPBUS_H__
#define __OPBUS_H__

typedef int (*opbus_cb)(unsigned char *req, int req_size, unsigned char *response, int res_size);
void *opbus_init(void);
void opbus_exit(void *bus);
int opbus_register(unsigned int type, opbus_cb cb);
int _opbus_send(unsigned int type, unsigned char *req, int size, unsigned char *response,int res_size);

#define opbus_send(type, req, size) _opbus_send(type, req, size, NULL, 0);
#define opbus_send_sync(type, req, size, response, res_size) _opbus_send(type, req, size, response, res_size);

#endif
