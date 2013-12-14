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







int test_get_mac_from_ip(int argc,char* argv[])
{
	struct mac_addr ma;
	struct in_addr ip;
	inet_aton(argv[1], &ip);
	if( get_mac_from_ip(&ip,&ma) == 0){
		printf("ETH0 Mac Addr: 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
				ma.mac[0],ma.mac[1],ma.mac[2],
				ma.mac[3],ma.mac[4],ma.mac[5]);
	}
	return 0;
}

int main(int argc,char* argv[])
{
	int sock;   //  socket fd.
	struct sockaddr sa;   // addr ourself.
	struct arp_package arp_pkg;  //arp pckage.
	struct mac_addr ma;
	struct ip_addr ip;
	char buf[100];

	struct in_addr src_in_addr, targ_in_addr;   //src addr, dest addr.
	test_get_mac_from_ip(argc,argv);
	return 0;


	sock = socket(AF_INET, SOCK_PACKET, htons(ETH_P_RARP));
	if(sock < 0)
	{
		fprintf(stderr,"Create Sockect Failed, with errno: %d\n",errno);
		exit(-1);
	}
	build_arp_pkg(&arp_pkg,0x01);
	//get_dev_mac(DEFAULT_DEVICE,&ma);
	get_host_ip(DEFAULT_DEVICE,&src_in_addr);

	if( get_mac_from_ip(argv[1],&ma)==0 ){
		printf("ETH0 Mac Addr: 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
				ma.mac[0],ma.mac[1],ma.mac[2],
				ma.mac[3],ma.mac[4],ma.mac[5]);
	}
	close(sock);
	return 0;
}
