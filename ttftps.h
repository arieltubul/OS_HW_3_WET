/*includes*/
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
#include <map>
#include <numeric>
//#include <chrono>
#include <ctime>
#include <string>

#define _SVID_SOURCE
#define _POSIX_C_SOURCE 200809L
#define DATA_PACKET_MAX_SIZE 516
#define MIN_PORT_NUM 10000
#define MAX_PORT_NUM 65535 //max number of 2 bytes
#define ERROR -1
#define PACKET_HEADER_SIZE 4
#define OP_BLOCK_FIELD_SIZE 2
#define PACKET_MAX_SIZE 1500
#define DATA_MAX_SIZE 512

using namespace std;
//typedef std::chrono::system_clock::time_point key_time;
typedef struct sockaddr* p_sock_addr; //original struct
typedef struct sockaddr_in sock_addr_in, *p_sock_addr_in; //organized struct for IPV4
typedef struct Ack_Packet ACK_P;
typedef struct Err_Packet ERR_P;
enum OPCODE {WRQ_OP = 2, DATA_OP = 3, ACK_OP =  4};
extern char* strdup(const char*);



/* structs */
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


/*************************         helper functions            ***********************/
/*declarations*/
void send_ack(p_sock_addr_in user_address, int user_address_size, int sock_fd, uint16_t ack_Num);
void send_error_msg(uint16_t errCode, const char* errMsg, p_sock_addr_in user_address, int user_address_size, int sock_fd);
bool check_WRQ_msg (char* Buffer);
void perror_func();


/*definitions*/
void send_ack(p_sock_addr_in user_address, int user_address_size, int sock_fd, uint16_t ack_Num)
{
    ACK_P current_ack;
    current_ack.block_num = htons(ack_Num);
    current_ack.opcode = htons(ACK_OP);
    if (sendto(sock_fd, (void*)(&current_ack), sizeof(current_ack), 0, (p_sock_addr)user_address, user_address_size) != sizeof(current_ack))
        perror_func();
}


void send_error_msg(uint16_t errCode, const char* errMsg, p_sock_addr_in user_address, int user_address_size, int sock_fd)
{
    ERR_P current_error;
    current_error.opcode = htons(5);
    current_error.errorCode = htons(errCode);
//    strcpy(current_error.errorMsg, errMsg);
    current_error.errorMsg = strdup(errMsg);
    *current_error.errorMsg = htons(*current_error.errorMsg);
    cout<<"error msg: "<<current_error.errorMsg<<endl;


    if (sendto(sock_fd, (void*)(&current_error), sizeof(current_error), 0, (p_sock_addr)user_address, user_address_size) != sizeof(current_error))
        perror_func();
}


/* valid wrq packet:
 * 1. 2 null characters, each in end of string
 * 2. valid file name (check by open)
 * 3. valid mode (octet)
 */
bool check_WRQ_msg (char* Buffer)
{
    //strdup copying string including null character
    char* filename = strdup(Buffer + OP_BLOCK_FIELD_SIZE);
    char* mode = strdup(Buffer + OP_BLOCK_FIELD_SIZE + strlen(filename) + 1);

    bool illegal_filename = false;
    for(int k=0; k<strlen(filename); k++) {
        if (filename[k] == '/') {
            illegal_filename = true;
            break;
        }
    }

    if ((filename == NULL)||(mode == NULL) || (strcmp(mode, "octet")) || (illegal_filename))
//        (Buffer[OP_BLOCK_FIELD_SIZE+strlen(filename)+1] != '\0') || (Buffer[OP_BLOCK_FIELD_SIZE+ strlen(filename)+1 + strlen(mode)+1] != '\0'))
    {
        cout<<"filename: "<<filename<<endl;
        cout<<"mode: "<<mode<<endl;
        free(filename);
        free(mode);
        return false;
    }
    else
        return true;
}


void perror_func()
{
    perror("TTFTP_ERROR");
    exit(ERROR);
}



/****************************           Client  Class        *********************************/
class Client
{
public:
    /*attributes*/
    sock_addr_in client_address; //ip and port
    uint16_t fails_num; //number of timeouts
    uint16_t last_block_num;
    char* file_name;
    int fd;
    time_t last_packet_time;

    /*methods*/
    Client(char* fileName, sock_addr_in client_add);
    ~Client();
};

Client::Client(char* fileName, sock_addr_in client_addr): fails_num(0), last_block_num(0)
{
    /* handling address */
    client_address.sin_family = client_addr.sin_family;
    client_address.sin_port = ntohs(client_addr.sin_port);
    client_address.sin_addr.s_addr = ntohl(client_addr.sin_addr.s_addr);

    last_packet_time = time(NULL);

    /* handling file */
    cout <<"Ctor 1"<<endl;
    file_name = strdup(fileName);
    cout <<"Ctor 2"<<endl;
    free(fileName); //fileName was created on heap by strdup in add_new_client, so after copying the name we need to free it
    fd = open(file_name, O_RDWR | O_TRUNC | O_CREAT, 0777); //create file with the same filename as original file with permission read,write&execute for owner,group,others
    cout <<"Ctor 3"<<endl;
    if (fd < 0)
        perror_func();
    cout <<"Ctor 4"<<endl;
}

Client::~Client(){} //we didn't allocate anything on heap so we D'tor is empty



/****************************         All_Clients   Class        *********************************/
class All_clients
{
public:
    /*attributes*/
    map<int, Client*> clientList;

    /*methods*/
    All_clients();
    ~All_clients();
    map<int, Client*>::iterator get_client(sock_addr_in client_addr);//for error #3
    bool file_exists(char* check_file); //for error #5
    void add_new_client(char* buffer, sock_addr_in curr_addr);
    map<int, Client*>::iterator get_earliest_packet_client();
//    bool operator<(const key_time& rhs);

};

All_clients::All_clients(){}

All_clients::~All_clients()
{
    map<int, Client*>::iterator it = clientList.begin();
    for (; it != clientList.end(); ) //erase automatically promotes it to next node
    {
        delete it->second;
        clientList.erase(it);
    }
}

map<int, Client*>::iterator All_clients::get_client(sock_addr_in client_addr)
{
    map<int, Client*>::iterator it = clientList.begin();
    for (; it != clientList.end(); it++)
    {
        if((it->second->client_address.sin_port == client_addr.sin_port) && (it->second->client_address.sin_addr.s_addr == client_addr.sin_addr.s_addr))
        {
            cout<<"I FOUND this client in get_client func"<<endl;
            cout<<" time IN FUNC is: "<<it->second->last_packet_time<<endl;
            return it;
        }
    }
    cout<<"I didnt find this client in get_client func"<<endl;
    cout<<" map size IN FUNC is: "<<clientList.size()<<endl;
    return clientList.end(); //return the element following the last element in map
}

bool All_clients::file_exists(char* check_file)
{
    FILE* test_fd = fopen(check_file, "r");
    if(test_fd != NULL) // we managed to open the file so its already exists
        return true;
    return false;
}

void All_clients::add_new_client(char* Buffer, sock_addr_in curr_addr) //included in create_client_from_WRQpacket(char* buffer)
{
//   1. parsing file name from buffer
//   2. calling C'tor of client
//   3. adding to map at current time
    cout <<"ok here 1"<<endl;
    char* filename = strdup(Buffer + OP_BLOCK_FIELD_SIZE); //need to free in client C'tor
    cout <<"ok here 2"<<endl;

    uint16_t port = ntohs(curr_addr.sin_port);
    unsigned long ip = ntohl(curr_addr.sin_addr.s_addr);
    char ip_string[64];
    char port_string[16];
    sprintf(port_string, "%hu", port);
    sprintf(ip_string, "%lu", ip);
    strcat(port_string, ip_string);
    int key = atoi(port_string);
//    int key = stoi(to_string(ip) + to_string(port));
    cout<<"calling ctor"<<endl;
    Client* new_cl = new Client(filename, curr_addr);
    clientList[key]= new_cl;
}

map<int, Client*>::iterator All_clients::get_earliest_packet_client()
{
    time_t min_time = time(NULL);//current time is bigger than any other time in map
    map<int, Client*>::iterator return_it = clientList.begin(); //iterator for element with the lowest time
    map<int, Client*>::iterator it = clientList.begin();
    for (; it != clientList.end(); it++)
    {
        if (it->second->last_packet_time <= min_time)
        {
            min_time = it->second->last_packet_time;
            return_it = it;
        }
    }
    return return_it;
}
//overloading for key in order to sort map wrt last time a message sent
//bool operator<(const key_time& lhs, const key_time& rhs)
//{
//    return lhs<rhs;
//}

//ostream& operator<<(ostream& os, const key_time& kt)
//{
//    os << " " <<kt;
//    return os;
//}

//    key_time rhs_copy = rhs;
//    std::chrono::duration<float> diff = rhs_copy - this->clientList.op;
//    std::chrono::duration<float> diff = rhs std::minus<float> this;

//    std::chrono::duration<double> diff = rhs - this;
//    if(diff>0) //difftime(end, begin) --> k2>k1
//    if(rhs  this)
//        return true;
//    return  false;
//}


#endif


