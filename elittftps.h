#include <iostream>
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
#include <iomanip>
#include <iostream>
#include <map>
#include <string.h>

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

using namespace std;
typedef struct sockaddr* p_sock_addr; //original struct
typedef struct sockaddr_in sock_addr_in, *p_sock_addr_in; //organized struct for IPV4
typedef struct Ack_Packet ACK_P, *PACK_P;
typedef struct WRQ_Msg WRQ_M, *PWRQ_P;
typedef struct Err_Packet ERR_P, *PERR_P;
enum OP_CODE {WRQ = 2, DATA = 3, ACK =  4};
//typedef struct timeval time;

void Add_Client_WRQ (char* Buffer, ALL_CLIENTS *all, sock_addr_in curr_add);
bool Check_WRQ_Msg (PWRQ_P msg, char* Buffer, ALL_CLIENTS *all);
void perror_func();
int SendAck(p_sock_addr_in User_address, int User_address_Size, int sock_Fd, uint16_t ack_Num);
int send_error_msg(uint16_t errCode, char* errMsg, p_sock_addr_in User_address, int User_address_Size, int sock_Fd);

//extern char* strdup(const char*); //FIXME: why not using in strcpy?


// struct of wrq packet
struct WRQ_Msg {
    char *filename;
    char *mode;
} __attribute__((packed));


// struct of ack packet
struct Ack_Packet {
    uint16_t opcode;
    uint16_t block_num;
} __attribute__((packed));

// struct of error packet
struct Err_Packet {
    uint16_t opcode;
    uint16_t errorCode;
    char *errorMsg;
} __attribute__((packed));



class Client
{
public:
    sock_addr_in client_address; //ip and port
    uint16_t fails_num, last_block_num;
    char *file_name;
    //bool finish; //checks if we finished client transaction

    //Client();
    Client(char *file, sock_addr_in client_add);
    ~Client();

};


Client::Client(char *file, sock_addr_in client_add):fails_num(0),last_block_num(0),file_name(NULL){
    //finish = false;
    this->file_name = new char[strlen(file)+1];
    strcpy(this->file_name, file);
    this->file_name[strlen(file)]='\0';
    (this->client_address).sin_family = client_add.sin_family;
    (this->client_address).sin_port=client_add.sin_port;
    (this->client_address).sin_addr.s_addr=client_add.sin_addr.s_addr;
    strcpy((this->client_address).sin_zero,client_add.sin_zero);

}
//typedef struct timeval time;

Client::~Client(){
    delete this->file_name;
}

class ALL_CLIENTS
{
    map<time_t, Client*> clientList;

    ALL_CLIENTS();
    ~ALL_CLIENTS();

    Client* getClient(sock_addr_in client_add);


    void addNewClient(char *file, sock_addr_in client_add);
    void deleteClient(sock_addr_in client_add);
    bool FileNameExists(char *check_file);
    //void setTime(sock_addr_in client_add, time_t currtime);


    map<time_t, Client*>::iterator getBegin();
    map<time_t, Client*>::iterator getEnd();
    map<time_t, Client*>::iterator Find(sock_addr_in client_add);

};

ALL_CLIENTS::ALL_CLIENTS(){}

ALL_CLIENTS::~ALL_CLIENTS(){
    map<time_t, Client*>::iterator it = clientList.begin();
    for (; it != clientList.end(); it++)
    {
        delete it->second;
    }

}



void ALL_CLIENTS::addNewClient(char *file, sock_addr_in client_add){
    Client *new_cl = new Client(file,client_add);
	time_t cur_time;
    time(&cur_time);
    clientList[cur_time]= new_cl;
}
void ALL_CLIENTS::deleteClient(sock_addr_in client_add){
    map<time_t, Client*>::iterator it = clientList.begin();
    for (; it != clientList.end(); it++){
        if ((it->second->client_address).sin_port == client_add.sin_port &&
            (it->second->client_address).sin_addr.s_addr==client_add.sin_addr.s_addr){
            delete it->second;
            clientList.erase(it);
        }
	}
}


map<time_t, Client*>::iterator ALL_CLIENTS::getBegin()
{
    return clientList.begin();
}

map<time_t, Client*>::iterator ALL_CLIENTS::getEnd()
{
    return clientList.end();
}

map<time_t, Client*>::iterator ALL_CLIENTS::Find(sock_addr_in client_add){
    map<time_t, Client*>::iterator it = clientList.begin();
    for (; it != clientList.end(); it++){
        if ((it->second->client_address).sin_port == client_add.sin_port &&
            (it->second->client_address).sin_addr.s_addr==client_add.sin_addr.s_addr){
            return it;
        }
    }
    return NULL;
}

Client* ALL_CLIENTS::getClient(sock_addr_in client_add){
    map<time_t, Client*>::iterator it = clientList.begin();
    for (; it != clientList.end(); it++){
        if ((it->second->client_address).sin_port == client_add.sin_port &&
            (it->second->client_address).sin_addr.s_addr==client_add.sin_addr.s_addr){
            return it->second;
        }
    }
    return NULL;
}

bool ALL_CLIENTS::FileNameExists(char *check_file){
    map<time_t, Client*>::iterator it = clientList.begin();
    for (; it != clientList.end(); it++){
        if (strcmp(it->second->file_name, check_file)==0){
            return true;
        }
    }
    return false;
}

/*void ALL_CLIENTS::setTime(sock_addr_in client_add, time_t currtime){
	map<time_t, Client*>::iterator it = clientList.Find(client_add);
	it->second
}*/

int send_error_msg(uint16_t errCode, char* errMsg,
p_sock_addr_in User_address, int User_address_Size, int sock_Fd){
    ERR_P current_error;
    current_error.opcode = htons(5);
    current_error.errorCode=htons(errorCode);
    current_error.errorMsg = new char[strlen(errMsg)+1];
    //TODO:to check if here we need htons
    strcpy(current_error.errorMsg,errMsg);
    current_error.errorMsg[strlen(errMsg)] = '\0';
    if (sendto(sock_Fd, (void*)(&current_error), sizeof(current_error), 0,
	(p_sock_addr)User_address, User_address_Size) != sizeof(current_error)){
        delete[] current_error.errorMsg;
        return 1;
    } else {
        delete[] current_error.errorMsg;
        return 0;
    }
}




void perror_func(){
        perror("TTFTP_ERROR");
        exit(ERROR);
}
    int SendAck(p_sock_addr_in User_address, int User_address_Size, int sock_Fd, uint16_t ack_Num) {

        ACK_P current_ack;
        current_ack.block_num = htons(ack_Num);
        current_ack.opcode = htons(ACK);
        if (sendto(sock_Fd, (void*)(&current_ack), sizeof(current_ack), 0, (p_sock_addr)User_address, User_address_Size) != sizeof(current_ack))
            return 1;
        else
            return 0;
    }
bool Check_WRQ_Msg (PWRQ_P msg, char* Buffer, ALL_CLIENTS *all) {
	uint16_t tmp_op_code = 0;
	memcpy(&tmp_op_code, Buffer, OP_BLOCK_FIELD_SIZE);

	// expected write requests opcode to be first
	if (ntohs(tmp_op_code) != WRQ)	{
		return false;
	}

	//get file from WRQ packet
	msg->filename = strdup(Buffer + OP_BLOCK_FIELD_SIZE);
	msg->mode = strdup(Buffer + OP_BLOCK_FIELD_SIZE + strlen(msg->filename) + 1);

	// check if succesful
	if ((msg->filename == NULL) || (msg->mode == NULL) ) {
		return false;
	}
	if (!(strcmp(msg->mode, "octet"))){
		return false;
	}
	if (Buffer[1+strlen(msg->filename+1)] !='\0'){
		return false;
	}
	if (Buffer[1+strlen(msg->filename+1+strlen(msg->mode)+1)!='\0'{
		return false;
	}
	if (all->FileNameExists(msg->filename)){
		return false;
	}
	return true;		
}

void Add_Client_WRQ(char* Buffer, ALL_CLIENTS *all, sock_addr_in curr_add){
	PWRQ_P msg;
	if (Check_WRQ_Msg(msg, Buffer, all)==true){
		all->addNewClient(msg->file_name, curr_add);
	}
}

#endif






