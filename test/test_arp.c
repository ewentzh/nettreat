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
#include <list.h>
#include <fifo.h>

struct testList{
	int id;
	struct list_head head;
};

int test_get_mac_from_ip(char* ipaddr)
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


int test_list()
{
	int i;
	struct testList *tmp;
	struct list_head* pos,*p;
	struct testList myTestList;

	INIT_LIST_HEAD(&myTestList.head);

	for(i=0; i<10; i++){
		tmp = malloc(sizeof(struct testList));
		tmp->id = i;
		/* add ahead! */
		list_add(&tmp->head,&myTestList.head);
	}

	for(i=10; i<15; i++){
		tmp = malloc(sizeof(struct testList));
		tmp->id = i;
		/* add tail! */
		list_add_tail(&tmp->head,&myTestList.head);
	}

	list_for_each(pos,&myTestList.head){
		tmp = list_entry(pos,struct testList,head);
		printf("List foreach: Id:  %d\n",tmp->id);
	}
	/* list_for_each_safe is only can be used as deteting. */
	list_for_each_safe(pos,p,&myTestList.head){
		tmp = list_entry(pos,struct testList,head);
		list_del(pos);
		free(tmp);
	}
	if(list_empty(&myTestList.head)){
		printf("Now List is empty!!\n");
	}
	return 0;
}


int test_fifo()
{
	int i;
	int test;
	int res,test1;
	struct kfifo fifo;
	res = kfifo_alloc(&fifo,1000);
	if(res!=0){
		printf("kfifo init failed!!\n");
		return -1;
	}

	test1 = 100;
	for(i=0; i<10; i++){
		test1 ++;
		res = kfifo_in(&fifo,&test1,sizeof(int));
		printf("Fifo In Data: %d\n",test1);
		if(res<=0){
			printf("kfifo in failed!!\n");
			return -1;
		}
	}

	for(i=0;i<5;i++){
		res = kfifo_out(&fifo,&test,sizeof(int));
		if(res<=0){
			printf("kfifo in failed!!\n");
			return -1;
		}
		printf("Out Fifo: %d\n",test);
	}

	printf( "Fifo Size: %d\n"
		    "Fifo in:  %d\n"
			"Fifo out: %d\n"
			"Fifo avail: %d\n"
			"Fifo data: %d\n",
			kfifo_size(&fifo),fifo.in,fifo.out,kfifo_avail(&fifo),kfifo_len(&fifo));

	return 0;
}
