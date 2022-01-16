#ifndef __ETH_H__
#define __ETH_H__
#include "skb_buff.h"
#include "opproto.h"

/** Internet protocol v4 */
#define OP_ETHTYPE_IP 0x0800
/** Address resolution protocol */
#define OP_ETHTYPE_ARP = 0x0806
/** Wake on lan */
#define OP_ETHTYPE_WOL = 0x0842
/** RARP */
#define OP_ETHTYPE_RARP = 0x8035
/** Virtual local area network */
#define OP_ETHTYPE_VLAN 0x8100
/** Internet protocol v6 */
#define OP_ETHTYPE_IPV6 0x86dd
/** PPP Over Ethernet Discovery Stage */
#define OP_ETHTYPE_PPPOEDISC 0x8863
/** PPP Over Ethernet Session Stage */
#define OP_ETHTYPE_PPPOE 0x8864
/** Jumbo Frames */
#define OP_ETHTYPE_JUMBO 0x8870
/** Process field network */
#define OP_ETHTYPE_PROFINET 0x8892
/** Ethernet for control automation technology */
#define OP_ETHTYPE_ETHERCATs 0x88a4
/** Link layer discovery protocol */
#define OP_ETHTYPE_LLDP 0x88cc
/** Serial real-time communication system */
#define OP_ETHTYPE_SERCOS 0x88cd
/** Media redundancy protocol */
#define OP_ETHTYPE_MRP 0x88e3
/** Precision time protocol */
#define OP_ETHTYPE_PTP 0x88f7
/** Q-in-Q, 802.1ad */
#define OP_ETHTYPE_QINQ 0x9100

int eth_decode(struct op_skb_buff *skb);

#endif

