#include "proto_decode.h"
#include "eth.h"

struct eth_proto_decode _eth_proto_decode [OP_ETH_TYPE_MAX] = {
	[OP_ETHTYPE_IP] = {.cb = ip_decode},
};

struct eth_proto_decode *get_eth_decode(unsigned short eth_type)
{
	return &_eth_proto_decode[eth_type];
}


