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


#define trace(fmt...) fprintf(stdout,"File:[%s] Line:[%d]## TRACE: %s",__FILE__,__LINE__,##fmt)
#define error(fmt...) fprintf(stdout,"File:[%s] Line:[%d]## TRACE: %s",__FILE__,__LINE__,##fmt)

#endif

