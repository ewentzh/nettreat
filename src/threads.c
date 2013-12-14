/*
 * threads.c
 *
 *  Created on: Dec 14, 2013
 *      Author: root
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <arp_trace.h>
#include <arp_thread.h>

//typedef void *(*start_routine)(void*);

pthread_t spawn_thread(start_routine func,void* param)
{
	int res;
	pthread_t thread_id;

	if(!func){
		error("thread func can be NULL\n");
		return -1;
	}
	res = pthread_create(&thread_id,NULL,func,param);
	if(res==0){
		return thread_id;
	}
	return -1;
}

