/*
 * arp.c
 *
 *  Created on: Dec 13, 2013
 *      Author: ewentzh
 */

#include <arp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <unistd.h>
#include <features.h> /* GLIBC VERSION */
#include <arp_trace.h>
#include <errno.h>
#if __GLIBC__ >= 2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h> /* L2 Definition */
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#endif

#define ASSERT(condition) assert(condition)


/**
 ** this function will get ipaddr from url. like www.baidu.com
 **
 **/
int get_ip_addr(struct in_addr* in_addr, char* str)
{
	struct hostent* hostp;
	in_addr->s_addr = inet_addr(str);
	if (in_addr->s_addr == -1) {
		if ((hostp = gethostbyname(str))) {
			bcopy(hostp->h_addr, in_addr, hostp->h_length);
		}
		else{
			error ("Can not get Ipaddr for host or IP:%s \n", str);
			return -1;
		}
	}
	return 0;
}

int create_arp_sock()
{
	int sock,res;
	struct mac_addr mac;
	struct sockaddr_ll myAddr;

	sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_RARP|ETH_P_ARP));
	if(sock<0){
		error("create arp socket error!!\n");
		return -1;
	}
	get_host_mac(DEFAULT_DEVICE,&mac);
	bzero(&myAddr,sizeof(myAddr));
	myAddr.sll_family = AF_PACKET;
	myAddr.sll_protocol=htons(ETH_P_ARP);
	myAddr.sll_hatype=ARPHRD_ETHER;
	myAddr.sll_pkttype=PACKET_HOST;
	myAddr.sll_halen=ETH_ALEN;
	myAddr.sll_ifindex = if_nametoindex(DEFAULT_DEVICE);
	memcpy(myAddr.sll_addr,&mac,ETH_HW_ADDR_LEN);

	res = bind(sock,(struct sockaddr *)&myAddr,sizeof(myAddr));
	if(res<0){
		close(sock);
		return -1;
	}
	return sock;
}

int destory_arp_sock(int sock)
{
	if(sock<0)
		return -1;
	return close(sock);
}

int send_arp(int sock,struct arp_package* arp_op,const char* devName)
{
	int res;
	struct sockaddr_ll to;

	bzero(&to,sizeof(to));
	to.sll_family = PF_PACKET;
	to.sll_ifindex = if_nametoindex(DEFAULT_DEVICE);
	if(!devName || !arp_op){
		return -1;
	}
	res = sendto(sock, arp_op, sizeof(struct arp_package), 0, (struct sockaddr*)&to, sizeof(to));
	if(res<0) {
		error("Send ARP Failed via DEV [%s]\n",devName);
		return -1;
	}
	return res;
}

int build_arp_pkg(struct arp_package* arp_op,unsigned short op_type)
{
	ASSERT(arp_op);
	memset((void*)arp_op,0,sizeof(struct arp_package));
	arp_op->frame_type = htons(ARP_FRAME_TYPE);
	arp_op->hw_type = htons(ETHER_HW_TYPE);
	arp_op->prot_type = htons(IP_PROTO_TYPE);
	arp_op->hw_addr_size = ETH_HW_ADDR_LEN;
	arp_op->ip_addr_size = IP_ADDR_LEN;
	arp_op->opration_code = htons(op_type);
	return 0;
}


int get_host_mac(const char* devName,struct mac_addr* mac)
{
	int sock_mac;
	struct ifreq ifr_mac;

	sock_mac = socket( AF_INET, SOCK_STREAM, 0 );
	if(sock_mac==-1)
	{
		error("create socket failed\n");
		return -1;
	}
	memset(&ifr_mac,0,sizeof(ifr_mac));
	strncpy(ifr_mac.ifr_name, devName, sizeof(ifr_mac.ifr_name)-1);
	if((ioctl(sock_mac,SIOCGIFHWADDR,&ifr_mac))<0) {
		error("mac ioctl error\n");
		return -1;
	}
	memcpy(mac,ifr_mac.ifr_hwaddr.sa_data,ETH_HW_ADDR_LEN);
	close(sock_mac);
	return 0;
}

int get_host_ip(const char* devName,struct in_addr* ipaddr)
{
	int sock_ip;
	struct ifreq ifr_ip;

	if(!devName||!ipaddr)
		return -1;

	sock_ip = socket( AF_INET, SOCK_STREAM, 0 );
	if(sock_ip==-1){
		error("create socket failed\n");
		return -1;
	}
	memset(&ifr_ip,0,sizeof(ifr_ip));
	strncpy(ifr_ip.ifr_name, devName, sizeof(ifr_ip.ifr_name)-1);
	if((ioctl(sock_ip,SIOCGIFADDR,&ifr_ip))<0) {
		error("mac ioctl error\n");
		return -1;
	}
	memcpy(ipaddr,&((struct sockaddr_in *)&ifr_ip.ifr_addr)->sin_addr,IP_ADDR_LEN);
	close(sock_ip);
	return 0;
}

/*******************************************************************************************
 * Description:
 *    this function will get Mac addr from System ARP table which learn by devName.
 *
 * Parameter:
 *    devName: input  - the device Name of Ethernet in System, such "eth0"
 *    ipaddr:  ipput  - the ip address which need to get.
 *    mac   :  output - output Mac Address.
 * Return Value:
 *    on Success: 0
 *    Failure:   -1  : when arp is not learn by System.
 *
********************************************************************************************/
int get_mac_from_dev(const char* devName,const struct in_addr* ipaddr,struct mac_addr* mac)
{
	int sockfd;
	struct sockaddr_in sin = {  0  };
	struct arpreq   myarp = { { 0 } };

	if(!ipaddr ||!devName||!mac) {
		error("Ipaddr or devName is NULL\n");
		return 0;
	}

	sin.sin_family = AF_INET;
#if 0
	/* only when the ipaddr is not a net address. */
	if(inet_aton(ipaddr, &sin.sin_addr)==0){
		error("Invalid Ip input:%s\n",ipaddr);
		return -1;
	}
#else
	/* use direct internet address. */
	memcpy(&sin.sin_addr,ipaddr,IP_ADDR_LEN);
#endif


	memcpy(&myarp.arp_pa, &sin, sizeof(myarp.arp_pa));
	strcpy(myarp.arp_dev, devName);
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		error("Create Sock Failed\n");
		return -1;
	}

	if (ioctl(sockfd, SIOCGARP, &myarp) == -1) {
		close(sockfd);
		return -1;
	}
	memset(mac,0,ETH_HW_ADDR_LEN);
	if(memcmp(mac,myarp.arp_ha.sa_data,ETH_HW_ADDR_LEN)!=0){
		memcpy(mac,myarp.arp_ha.sa_data,ETH_HW_ADDR_LEN);
		return 0;
	}
	return -1;
}

int get_mac_from_arp(const struct in_addr* ipaddr,struct mac_addr* mac)
{
	int sock = -1;
	int len, res = 0;
	struct in_addr ip;
	struct sockaddr_ll from,to,myAddr;
	char buffer[ETH_FRAME_LEN]={0};
	struct arp_package arp_req,*arp_res_op;  //arp pckage.

	if( build_arp_pkg(&arp_req,OP_ARP_REQUEST) !=0 ){
		res = -1;
		error("build arp package error!!\n");
		goto arp_out;
	}

	if( get_host_mac(DEFAULT_DEVICE,(struct mac_addr* )arp_req.src_hw_addr)!=0 ){
		res = -1;
		error("get host mac addr error!!\n");
		goto arp_out;
	}
	memset(arp_req.targ_hw_addr,0xff,ETH_HW_ADDR_LEN);
	memcpy(arp_req.sndr_hw_addr,arp_req.src_hw_addr,ETH_HW_ADDR_LEN);

	/* Here get the internet ip address */
	if( get_host_ip(DEFAULT_DEVICE,(struct in_addr *)arp_req.sndr_ip_addr) !=0) {
		res = -1;
		error("get host ip addr error!!\n");
		goto arp_out;
	}
#if 0
	/* only when the */
	if(inet_aton(ipaddr, &ip)==0){
		res=-1;
		goto arp_out;
	}
#else
	memcpy(&ip,ipaddr,IP_ADDR_LEN);
#endif
	memcpy(arp_req.rcpt_ip_addr,&ip,IP_ADDR_LEN);

	/* Send Arp, and Receive it with a time out. */
	sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_RARP|ETH_P_ARP));
	if(sock < 0){
		res = -1;
		error("create socket error!! with errno=%d\n",errno);
		goto arp_out;
	}
	bzero(&myAddr,sizeof(myAddr));
	myAddr.sll_family = AF_PACKET;
	myAddr.sll_protocol=htons(ETH_P_ARP);
	myAddr.sll_hatype=ARPHRD_ETHER;
	myAddr.sll_pkttype=PACKET_HOST;
	myAddr.sll_halen=ETH_ALEN;
	myAddr.sll_ifindex = if_nametoindex(DEFAULT_DEVICE);
	memcpy(myAddr.sll_addr,arp_req.src_hw_addr,ETH_HW_ADDR_LEN);

	res = bind(sock,(struct sockaddr *)&myAddr,sizeof(myAddr));
	if(res < 0){
		res=-1;
		error("bind sock to local address error!!\n");
		goto arp_out;
	}

	bzero(&to,sizeof(to));
	to.sll_family = PF_PACKET;
	to.sll_ifindex = if_nametoindex(DEFAULT_DEVICE);
#ifdef DEBUG
	dump_arp(&arp_req);
#endif

	len = sendto(sock,(void*)&arp_req,sizeof(arp_req),0,(struct sockaddr*)&to,sizeof(to));
	if(len<0){
		res = -1;
		goto arp_out;
	}

	/* Receive ARP:*/
	res = recvfrom(sock,buffer,ETH_FRAME_LEN,0,(struct sockaddr*)&from,(socklen_t*)&len);
	if(res<0){
		res = -1;
		error("Recv Error: %d\n",errno);
		goto arp_out;
	}
	res = 0;
	arp_res_op = (struct arp_package*)buffer;
	memcpy(mac,arp_res_op->src_hw_addr,ETH_HW_ADDR_LEN);
#ifdef DEBUG
	dump_arp(arp_res_op);
#endif
arp_out:
	if(sock>0){
		close(sock);
		sock = 0;
	}
	return res;
}

int get_mac_from_ip(const struct in_addr* ipaddr,struct mac_addr* mac)
{
	int res = 0;

	/*TODO: Fix later, here just to get mac from eth0 devices. */
	res = get_mac_from_dev(DEFAULT_DEVICE,ipaddr,mac);
	if( res == 0)
		return 0;

	/* Here Send a Arp to get Mac addr!! */
	return get_mac_from_arp(ipaddr,mac);
}

void dump_arp(struct arp_package * arp_op)
{
	ASSERT(arp_op);
	printf("Dump ARP Package:  \n"
			"   Target HW Mac    : 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n"
			"   Source HW Mac    : 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n"
			"   ARP Ops  code    : %s (%d) \n"
			"   ARP Sender Mac   : 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n"
			"   ARP Sender IP    : %d.%d.%d.%d\n"
			"   ARP Receiver Mac : 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n"
			"   ARP Receiver IP  : %d.%d.%d.%d\n",
			arp_op->targ_hw_addr[0],arp_op->targ_hw_addr[1],arp_op->targ_hw_addr[2],arp_op->targ_hw_addr[3],arp_op->targ_hw_addr[4],arp_op->targ_hw_addr[5],
			arp_op->src_hw_addr[0],arp_op->src_hw_addr[1],arp_op->src_hw_addr[2],arp_op->src_hw_addr[3],arp_op->src_hw_addr[4],arp_op->src_hw_addr[5],
			(arp_op->opration_code ==OP_ARP_REQUEST) ? "ARP REQUEST" : "ARP RESPONSE",arp_op->opration_code,
			arp_op->sndr_hw_addr[0],arp_op->sndr_hw_addr[1],arp_op->sndr_hw_addr[2],arp_op->sndr_hw_addr[3],arp_op->sndr_hw_addr[4],arp_op->sndr_hw_addr[5],
			arp_op->sndr_ip_addr[0],arp_op->sndr_ip_addr[1],arp_op->sndr_ip_addr[2],arp_op->sndr_ip_addr[3],
			arp_op->rcpt_hw_addr[0],arp_op->rcpt_hw_addr[1],arp_op->rcpt_hw_addr[2],arp_op->rcpt_hw_addr[3],arp_op->rcpt_hw_addr[4],arp_op->rcpt_hw_addr[5],
			arp_op->rcpt_ip_addr[0],arp_op->rcpt_ip_addr[1],arp_op->rcpt_ip_addr[2],arp_op->rcpt_ip_addr[3]);
}
