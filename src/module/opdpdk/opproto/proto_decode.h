#ifndef _PROTO_DECODE_H__
#define _PROTO_DECODE_H__
#include "opproto.h"
#include "ip.h"

struct eth_proto_decode {
	eth_type_cb cb;
};

struct eth_proto_decode *get_eth_decode(unsigned short eth_type);

#endif
