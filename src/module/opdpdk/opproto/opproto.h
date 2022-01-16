#ifndef __OPPROTO_H__
#define __OPPROTO_H__
#define OP_ETH_ALEN 6
#define OP_ETH_TYPE_MAX 65536

struct op_skb_buff;

typedef int (*eth_type_cb)(struct op_skb_buff *);

struct op_ethhder {
	unsigned char	h_dest[OP_ETH_ALEN];
	unsigned char	h_source[OP_ETH_ALEN];
	unsigned short h_proto;
}__attribute__((packed));

struct op_skb_buff
{
	unsigned char *frame;
	unsigned int frame_len;
	struct op_ethhder *eth;
	unsigned int eth_len;
	unsigned short eth_type;
};

#endif


