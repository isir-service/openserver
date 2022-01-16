#include <arpa/inet.h>
#include "stdio.h"

#include "eth.h"
#include "base/oplog.h"
int eth_decode(struct op_skb_buff *skb)
{
	if (skb->frame_len < sizeof(struct op_ethhder)) {
		log_warn_ex("frame len less then eth\n");
		goto out;
	}

	skb->eth = (struct op_ethhder *)skb->frame;
	skb->eth_len = sizeof(struct op_ethhder);
	skb->eth_type = ntohs(skb->eth->h_proto);

	return 0;
out:
	return -1;
}

