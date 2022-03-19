#ifndef __IP_H__
#define _IP_H__
#include "opproto.h"

#define IP_DF		0x4000		/* Flag: "Don't Fragment"	*/
#define IP_MF		0x2000		/* Flag: "More Fragments"	*/
#define IP_OFFSET	0x1FFF		/* "Fragment Offset" part	*/

#define IPV4_HEADER_NORMAL_SIZE 20

struct op_skb_buff * ip_decode(struct op_skb_buff *skb, int hash);
int ip_frag(struct op_skb_buff *skb, struct op_skb_buff *skb_new);

#endif

