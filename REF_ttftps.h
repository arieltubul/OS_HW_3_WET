#ifndef _TTFTPS_H_
#define _TTFTPS_H_

#define _SVID_SOURCE
#define _POSIX_C_SOURCE 200809L
extern char* strdup(const char*); //FIXME: why not using in strcpy?

enum OP_CODE {WRQ = 2, DATA = 3, ACK =  4};

#define PACKET_MAX_SIZE 516
#define NOT_DONE 1 
#define DONE 2

#define MIN_PORT_NUM 10000
#define MAX_PORT_NUM 65535

#define NUMBER_OF_FAILURES 7
#define WAIT_FOR_PACKET_TIMEOUT 3;
#define ERROR -1
#define PACKET_HEADER_SIZE 4
#define OP_BLOCK_FIELD_SIZE 2
#define DATA_MAX_SIZE 512

typedef struct sockaddr_in Sock_Addr_In, *PSock_Addr_In;
typedef struct Ack_Packet ACK_P, *PACK_P;
typedef struct WRQ_Msg WRQ_M, *PWRQ_P;
typedef struct timeval time;
typedef struct sockaddr *PSock_addr;

// struct of wrq packet
struct WRQ_Msg { //TODO: what about opcode? we may add it to struct?
	char *filename;
	char *mode;
} __attribute__((packed));


// struct of ack packet
struct Ack_Packet {
	uint16_t opcode;
	uint16_t block_num;
} __attribute__((packed));



void p_error(char* msg) {
	perror(msg);
	exit(ERROR);
}

void error(char* msg) {
	printf("%s", msg);
	exit(ERROR);
}

// checking all arguments are valid, if so: write to dstfile the msg from client 
// in case of failure return error and print to perror
int Data_Packet_Handler(char* Buffer, int dst_file, uint16_t ack_Num, int msg_Size) {
	int Data_field_size = msg_Size - PACKET_HEADER_SIZE;
	uint16_t opcode = 0, block_num = 0;
	char* data;
	if (Data_field_size < 0)
		error("FLOWERROR: wrong Data_field_size\n");
	memcpy(&opcode, Buffer, OP_BLOCK_FIELD_SIZE);
	if (ntohs(opcode) != DATA)
		error("FLOWERROR: wrong opcode\n");
	memcpy(&block_num, Buffer + OP_BLOCK_FIELD_SIZE, OP_BLOCK_FIELD_SIZE); //Buffer[OP_BLOCK_FIELD_SIZE] = Buffer[2] <-- block number
	if (ack_Num != ntohs(block_num))
		error("FLOWERROR: wrong block number\n");
	printf("IN:DATA,%d,%d\n", ntohs(block_num), msg_Size);
	if (msg_Size == PACKET_HEADER_SIZE) return DONE; // no data (sizeof_fileֵ % mod 512 == 0)

	data = (char*)malloc(Data_field_size * sizeof(char));
	if (!data)
		p_error("TTFTP_ERROR: memory allocation failure \n");//FIXME: MALLOC IS NOT A SYSTEM CALL
	memcpy(data, Buffer + PACKET_HEADER_SIZE, Data_field_size);
	printf("WRITING: %d\n", Data_field_size);
	if (write(dst_file, data, Data_field_size) < Data_field_size) {
		free(data);
		p_error("TTFTP_ERROR: failure writing to destination \n");
	}
	free(data);
	if (Data_field_size < DATA_MAX_SIZE)  //meaning this is the last packet
		return DONE;
	return NOT_DONE;
}

//init ack packet and sending it to client, in case of success print as requires, else
//in case of failure return error and print to perror

int Ack_Packet_Handler(PSock_Addr_In User_address, int User_address_Size, int sock_Fd, uint16_t ack_Num) {

	ACK_P current_ack;
	current_ack.block_num = htons(ack_Num);
	current_ack.opcode = htons(ACK);

	if (sendto(sock_Fd, (void*)(&current_ack), sizeof(current_ack), 0, (PSock_addr)User_address, User_address_Size) != sizeof(current_ack))
		p_error("TTFTP_ERROR: sendto failure\n");
	printf("OUT:ACK, %d\n", ack_Num);
	return 0;
}

void set_server_add(int port_num, PSock_Addr_In tftp_add) {
	tftp_add->sin_family = AF_INET;
	tftp_add->sin_addr.s_addr = htonl(INADDR_ANY);
	tftp_add->sin_port = htons(port_num);
}

int Convert_Buffer_to_WRQ (PWRQ_P msg, char* Buffer) {
	uint16_t tmp_op_code = 0;
	memcpy(&tmp_op_code, Buffer, OP_BLOCK_FIELD_SIZE);

	// expected write requests opcode to be first
	if (ntohs(tmp_op_code) != WRQ)	{
		return 1;
	}

	//get file from WRQ packet
	msg->filename = strdup(Buffer + OP_BLOCK_FIELD_SIZE);
	msg->mode = strdup(Buffer + OP_BLOCK_FIELD_SIZE + strlen(msg->filename) + 1); //+1 cause we have the string terminator in size of 1byte

	// check if succesful
	if ((msg->filename == NULL) || (msg->mode == NULL) ) {
		return 1;
	}

	return 0;		
}

#endif // ! ֹ_ֹTTֹFTPS_H_

