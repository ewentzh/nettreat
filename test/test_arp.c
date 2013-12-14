/*
 * test_arp.c
 *
 *  Created on: Dec 14, 2013
 *      Author: root
 */


#include <arp.h>
#include <arp_trace.h>
#include <stdlib.h>
#include <stdio.h>


int test_get_mac_from_ip(struct ip_addr* ipaddr)
{
	struct mac_addr ma;
	struct in_addr ip;
	inet_aton(ipaddr, &ip);
	if( get_mac_from_ip(&ip,&ma) == 0){
		printf("ETH0 Mac Addr: 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\n",
				ma.mac[0],ma.mac[1],ma.mac[2],
				ma.mac[3],ma.mac[4],ma.mac[5]);
	}
	return 0;
}
