/*
 ============================================================================
 Name        : arp.h
 Author      : ewentzh
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */

#ifndef __ARP__HEADER_NET_TREAT__
#define __ARP__HEADER_NET_TREAT__

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <linux/if_ether.h>

#define IP_ADDR_LEN 4
#define ETH_HW_ADDR_LEN 6

#define ARP_FRAME_TYPE 0x0806
#define ETHER_HW_TYPE 1
#define IP_PROTO_TYPE 0x0800

#define OP_ARP_REQUEST 1
#define OP_ARP_RESPONSE 2

#define DEFAULT_DEVICE "eth0"

struct ip_addr {
	unsigned char ipaddr[IP_ADDR_LEN];
};

struct mac_addr{
	unsigned char mac[ETH_HW_ADDR_LEN];
};


struct arp_package {
	/* receiver mac adddress */
	unsigned char targ_hw_addr[ETH_HW_ADDR_LEN];

	/* sender mac address */
	unsigned char src_hw_addr[ETH_HW_ADDR_LEN];

	/* Ethertype - 0x0806 is the ARP Frame Type */
	unsigned short frame_type;

	/* HW type: 0x01 is for Ethernet. */
	unsigned short hw_type;

	/* upper level potoco type, 0x01 is for IP */
	unsigned short prot_type;

	/* HW Addr size */
	unsigned char hw_addr_size;

	/* ip address size */
	unsigned char ip_addr_size;

	/* operation code - 0x01 for request, 0x02 response */
	unsigned short opration_code;

	/* sender Hw Mac */
	unsigned char sndr_hw_addr[ETH_HW_ADDR_LEN];
	/* Sender IP addr */
	unsigned char sndr_ip_addr[IP_ADDR_LEN];

	/* Receiver HW Mac */
	unsigned char rcpt_hw_addr[ETH_HW_ADDR_LEN];

	/* Receiver IP address */
	unsigned char rcpt_ip_addr[IP_ADDR_LEN];

	/* Padding */
	unsigned char padding[18];
};

extern int build_arp_pkg(struct arp_package* arp_op,unsigned short op_type);
extern int send_arp(int sock,struct arp_package* arp_op,const char* devName);
extern int get_ip_addr(struct in_addr* ip, char* str);
extern int get_host_mac(const char* devName,struct mac_addr* mac);
extern int get_host_ip(const char* devName,struct in_addr* ipaddr);
extern int get_mac_from_ip(const struct in_addr* ipaddr,struct mac_addr* mac);
extern void dump_arp(struct arp_package * arp_op);
extern int create_arp_sock();
extern int destory_arp_sock(int sock);

#endif
