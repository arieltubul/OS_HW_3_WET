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
#define DATA_PACKET_MAX_SIZE 516

#define NOT_DONE 1
#define DONE 2
#define MIN_PORT_NUM 10000
#define MAX_PORT_NUM 65535 //max number of 2 bytes
#define ERROR -1
#define PACKET_HEADER_SIZE 4
#define OP_BLOCK_FIELD_SIZE 2
#define PACKET_MAX_SIZE 1500
#define DATA_MAX_SIZE 512

typedef struct sockaddr sock_addr, *p_sock_addr; //original struct
typedef struct sockaddr_in sock_addr_in, *p_sock_addr_in; //organized struct for IPV4
typedef struct Ack_Packet ACK_P, *PACK_P;
typedef struct WRQ_Msg WRQ_M, *PWRQ_P;

struct Ack_Packet
{
    uint16_t opcode;
    uint16_t block_num;
}__attribute__((packed));


class Client //single client
{
    //attributes:

    //methods:
};
class Clients_map
{

};



extern char* strdup(const char*); //FIXME: why not using in strcpy?

enum OPCODE {WRQ_OP = 2, DATA_OP = 3, ACK_OP =  4};

using namespace std;

void perror_func()
{
    perror("TTFTP_ERROR");
    //we need also to free all allocations (maybe map, sock of server...)
    exit(ERROR);
}

//overloading for key in order to sort map wrt last time a message sent
bool operator<(const time_t& k1, const time_t& k2)
{
    if(difftime(k2, k1) > 0) //difftime(end, begin) --> k2>k1
        return true;
    return  false;
}
bool is_wrq_packet_valid(char* packet)
{

}

void special_erase(Client client)
}


////overloading access or adding to map - NO NEED BECAUSE WE OVERLOADED OPERATOR<
//Client& operator[](const time_t& key)
//{
//    re
//}

#endif