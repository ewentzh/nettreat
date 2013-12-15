/*
 ============================================================================
 Name        : arp.h
 Author      : ewentzh
 Version     :
 Copyright   :
 Description :
 ============================================================================
 */


#ifndef __TRACE_ARP_HEADER_FILE__
#define __TRACE_ARP_HEADER_FILE__


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define trace(fmt,...) fprintf(stdout,"File:[%s] Line:[%d] trace##:" fmt,\
		           strrchr(__FILE__,(int)'/')==NULL?__FILE__:strrchr(__FILE__,(int)'/')+1,__LINE__,##__VA_ARGS__)

#define error(fmt,...) fprintf(stdout,"File:[%s] Line:[%d] error##:" fmt,\
		           strrchr(__FILE__,(int)'/')==NULL?__FILE__:strrchr(__FILE__,(int)'/')+1,__LINE__,##__VA_ARGS__)

#endif

