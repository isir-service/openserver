#ifndef __U9300C_H__
#define __U9300C_H__
#include "cell_pub.h"


int u9300_init(struct lte_module_info *module);

int u9300_send_text_message(char *phone_num, char * message, unsigned short size);
int u9300_send_pdu_message(char *phone_num, char *phone_center, char * message, unsigned short size);

#endif
