#ifndef __IP_H__
#define _IP_H__
#include "opproto.h"

int ip_decode(struct op_skb_buff *skb);
int ip_frag(struct op_skb_buff *skb, struct op_skb_buff *skb_new);

#endif

