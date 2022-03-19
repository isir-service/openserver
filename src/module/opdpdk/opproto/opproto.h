#ifndef __OPPROTO_H__
#define __OPPROTO_H__

#include <sys/time.h>
#include <time.h>
#include "opbox/oplist.h"

#define OP_ETH_ALEN 6
#define OP_ETH_TYPE_MAX 65536
#define OP_IP_MAX_SIZE  65535
#define IP_FRAG_CONTINUE 1
#define IP_FRAG_OVER 2
#define IP_FRAG_TIMEOUT_CHECK_S 10
#define IP_FRAG_TIMEOUT 20

#define OP_IP_PROTO_MAX 256

struct op_skb_buff;

typedef struct op_skb_buff *(*eth_type_cb)(struct op_skb_buff *, int);
typedef int (*ip_proto_cb)(struct op_skb_buff *);

struct op_ethhder {
	unsigned char	h_dest[OP_ETH_ALEN];
	unsigned char	h_source[OP_ETH_ALEN];
	unsigned short h_proto;
}__attribute__((packed));

struct op_iphdr {
#if __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
	unsigned char version:4,
					ihl:4;
#elif __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
	unsigned char ihl:4,
		version:4;
#else
#error	"Please fix <asm/byteorder.h>"
#endif
	unsigned char tos;
	unsigned short tot_len;
	unsigned short id;
	unsigned short frag_off;
	unsigned char ttl;
	unsigned char protocol;
	unsigned short check;
	unsigned int saddr;
	unsigned int daddr;
}__attribute__((packed));

struct ip_frag_data {
	unsigned char *data;
	unsigned short data_len;
	unsigned short offset;
	struct ip_frag_data *next;
};

struct ip_frag_info {
	unsigned char frag:1,
				 over:1,
				 first_find:1,
				last_pkt:1;
	unsigned short last_pkt_offlen;
	unsigned short frag_write_size;
	struct ip_frag_data *data_head;
	struct ip_frag_data *data_tail;
	unsigned int time;
};

struct op_skb_buff
{
	unsigned char *frame;
	unsigned int frame_len;
	struct op_ethhder *eth;
	unsigned int eth_len;
	unsigned short eth_type;
	struct op_iphdr *iphdr;
	unsigned char iphdr_len;
	unsigned char *ip_option;
	unsigned char ip_option_length;
	struct ip_frag_info ip_frag;

	//private
	struct timeval ts;
	void *work_thread;
};

struct ip_frag_timeout {
	struct op_skb_buff *skb;
	struct oplist_head list;
};

#endif


