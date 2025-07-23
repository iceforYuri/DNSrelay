#ifndef DNSHANDLE_H
#define DNSHANDLE_H

#include "header.h"
#include "multiThread.h"
#include "dns_message.h"
#include "data_struct.h"
#include "platformSocket.h"
#include "debug.h"

extern my_socket servSock;
extern struct sockaddr_in remoteSockAddr;
extern int message_count;

void DNSHandle(Task *t);

#endif