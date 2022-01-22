#ifndef __OPPROTO_H__
#define __OPPROTO_H__

#include <sys/time.h>
#include <time.h>

#define OP_ETH_ALEN 6
#define OP_ETH_TYPE_MAX 65536
#define IPV4_HEADER_NORMAL_SIZE 20
#define OP_IP_MAX_SIZE  65535
#define IP_FRAG_CONTINUE 1
#define IP_FRAG_OVER 2
#define IP_FRAG_TIMEOUT_S 60

#define OP_IP_PROTO_MAX 256

struct op_skb_buff;

typedef int (*eth_type_cb)(struct op_skb_buff *);
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

struct ip_frag_info {
	unsigned char ip_df:1,
				  ip_mf:1;
	unsigned short ip_frag_offset;

//private
	unsigned short frag_write_size;
	unsigned char *frag_buf_l4;
	unsigned short frag_buf_size;
	unsigned short last_offset;
	unsigned short last_length;
};

struct op_skb_buff
{
	unsigned char *frame;
	unsigned int frame_len;
	struct op_ethhder *eth;
	unsigned int eth_len;
	unsigned short eth_type;
	struct op_iphdr *iphdr;
	unsigned int iphdr_len;
	unsigned char *ip_option;
	unsigned int ip_option_length;
	struct ip_frag_info ip_frag;

	//private
	struct timeval ts;
};

#endif


