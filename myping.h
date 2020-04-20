/*********************************************************************************
  *FileName: myping.h
  *Author: Lan Qiao
  *Date: 04/19/2020
  *Description: Header of ping program in C.
**********************************************************************************/

#ifndef _myping_h_
#define _myping_h_

#include <arpa/inet.h>
#include <math.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define ICMP_SIZE (sizeof(struct icmphdr))
#define BUF_SIZE 1024

// ping
void ping();

// pack icmp
void pack(struct icmphdr *, int);

// unpack
int unpack(uint8_t *, int, const uint8_t *);

// calculate checksum
uint16_t checksum(uint16_t *, int);

// calculate time difference
double timediff(struct timeval *, struct timeval *);

// print statistics
void statistics();

// handle ctrl+c
void handler();

#endif