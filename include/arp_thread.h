/*
 * threads.c
 *
 *  Created on: Dec 14, 2013
 *      Author: root
 */

#ifndef __THREAD_ARP_HEADER__
#define __THREAD_ARP_HEADER__

#include <pthread.h>

typedef void* (*start_routine) (void*);
extern pthread_t spawn_thread(start_routine func,void* param);

#endif
