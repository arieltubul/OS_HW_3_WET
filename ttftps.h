#ifndef _TTFTPS_H_
#define _TTFTPS_H_
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <assert.h>
#include <iostream>

#define _SVID_SOURCE
#define _POSIX_C_SOURCE 200809L
#define PACKET_MAX_SIZE 516
#define NOT_DONE 1
#define DONE 2
#define MIN_PORT_NUM 10000
#define MAX_PORT_NUM 65535 //max number of 2 bytes
#define ERROR -1
#define PACKET_HEADER_SIZE 4
#define OP_BLOCK_FIELD_SIZE 2
#define DATA_MAX_SIZE 512

typedef struct sockaddr* p_sock_addr; //original struct
typedef struct sockaddr_in sock_addr_in, *p_sock_addr_in; //organized struct for IPV4
typedef struct Ack_Packet ACK_P, *PACK_P;
typedef struct WRQ_Msg WRQ_M, *PWRQ_P;
typedef struct timeval time;

class Clients
{
    //attributes:

    //methods:
};
//typedef struct timeval time;



extern char* strdup(const char*); //FIXME: why not using in strcpy?

enum OP_CODE {WRQ = 2, DATA = 3, ACK =  4};

using namespace std;

void perror_func()
{
    perror("TTFTP_ERROR");
    exit(ERROR);
}

#endif