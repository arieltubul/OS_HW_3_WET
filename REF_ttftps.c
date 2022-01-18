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
#include "ttftps.h"

int main(int argc, char** argv)//argv[0]=name of program, argv[1]=portNum
{
	// input check - see if we have 2 arg
	if (argc != 2)
		error("TTFTP_ERROR: invalid amount of arguments\n");
	
	int sock;
	int is_packet_done = NOT_DONE;
	int port_num = atoi(argv[1]);
	int num_of_ack;
	int retval_select;
	int timeoutExpiredCount = 0;

	char Buffer[PACKET_MAX_SIZE] = { 0 };

	Sock_Addr_In tftp_add = { 0 };
	Sock_Addr_In User_address = { 0 };
	socklen_t get_len_client;

	WRQ_M wrq_msg;

	bool Error_flag = false;
	struct timeval timeout;
	fd_set read_fds;

	if (port_num < MIN_PORT_NUM || port_num > MAX_PORT_NUM) {
		fprintf(stderr, "TTFTP_ERROR - Usage: %s port number should be in range[10000, 65535]\n", argv[0]);
		exit(1);
	}

	// -----------------------  server setup like recition 9 -----------------------

	// opening the socket and checking if succesful
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) //TODO: why not using 0 instead of IPPROTO_UDP
		p_error("TTFTP_ERROR: create socket failure\n");

	memset(&tftp_add, 0, sizeof(tftp_add)); //TODO: why write 0 again after doing it in line 30

	// init the server
	set_server_add(port_num, &tftp_add); 

	// binding the socket
	if (bind(sock, (PSock_addr) & tftp_add, (socklen_t)sizeof(tftp_add)) < 0)	{
		close(sock);
		p_error("TTFTP_ERROR: bind failure\n");
	}

	//init timeval struct
	timeout.tv_sec = WAIT_FOR_PACKET_TIMEOUT;
	timeout.tv_usec = 0;

	// for loop as long as the server is needed
	for (; !Error_flag;) { //equivalent to  while(Error_flag)
		timeoutExpiredCount = 0;
		get_len_client = sizeof(User_address);
		num_of_ack = 0;

		// receive packet and check if successful
		if (recvfrom(sock, Buffer, PACKET_MAX_SIZE, 0, (PSock_addr) & User_address, &get_len_client) < 0)	{
			perror("TTFTP_ERROR: recvfrom failure\n");
			Error_flag = true;
			break;
		}

		if (Convert_Buffer_to_WRQ(&wrq_msg, Buffer)) { //TODO: maybe filename or mode are NULL?
			printf("FLOWERROR: wrong opcode, expected write requests opcode to be first\n");
			printf("RECVFAIL\n");
			continue;
		}

		// open the file
		int dest_file = open(wrq_msg.filename, O_RDWR | O_TRUNC | O_CREAT, 0777); //EQUIVALENT TO CREATE FILE WITH PERMSSION TO read, write, & execute for owner, group and others
		if (dest_file < 0) {
			free(wrq_msg.filename);
			free(wrq_msg.mode);
			perror("TTFTP_ERROR: open failure\n");
			Error_flag = true;
			break;
		}

		printf("IN:WRQ,%s,%s\n", wrq_msg.filename, wrq_msg.mode);
		free(wrq_msg.filename);
		free(wrq_msg.mode);
		
		if (Ack_Packet_Handler(&User_address, get_len_client, sock, num_of_ack) == ERROR) { //didnt manage to send back ACK packet to client
			close(dest_file);
			Error_flag = true;
			break;
		}

		num_of_ack++;
        //NOTICE: every time we set ERROR_FLAG = true we exit from program
		do {
			do {
				do {
                    //TODO: if we clear the read_fds by FD_ZERO why we need also to remove by FD_CLR?
					FD_ZERO(&read_fds); //clears the read_fds
					FD_CLR(sock, &read_fds); //removes sock file descriptor from read_fds
					FD_SET(sock, &read_fds); //adds file descriptor to the set

					retval_select = select((sock + 1), &read_fds, NULL, NULL, &timeout);
					timeout.tv_sec = WAIT_FOR_PACKET_TIMEOUT; //TODO: we have already done it in line 63
					timeout.tv_usec = 0;
                    //fd_isset checks if the neew soc is now part of the set
                    //retval select > 0 if we got any data before timout
					if (FD_ISSET(sock, &read_fds) && retval_select > 0) {
						int get_size_of_data = recvfrom(sock, Buffer, PACKET_MAX_SIZE, 0, (PSock_addr)&User_address, &get_len_client);
                        // There is no data to read
						if (get_size_of_data < PACKET_HEADER_SIZE) {
						// Fail
							if (get_size_of_data < 0)
								perror("TTFTP_ERROR: recvfrom failure\n");
							else //receive succeded but header is not legal
								perror("TTFTP_ERROR: couldnt get HEADER\n");
							close(dest_file);
							Error_flag = true;
							break;
						}

						is_packet_done = Data_Packet_Handler(Buffer, dest_file, num_of_ack, get_size_of_data);
						if (is_packet_done == NOT_DONE) {
						//success and we have more to read
							if (Ack_Packet_Handler(&User_address, get_len_client, sock, num_of_ack) == ERROR) {
								printf("FLOWERROR: failure sending ACK %d\n", num_of_ack);
								close(dest_file);
								Error_flag = true;
								break;
							}
							num_of_ack++;
							continue;
						}
						else if (is_packet_done == ERROR) {
							close(dest_file);
							Error_flag = true;
							break;
						}
						else // we finish with current client
							break;
					}

					else if (retval_select == 0) { //there was timout
						++timeoutExpiredCount;
						printf("FLOWERROR: timeout occured, sending previous ack again\n");
                        //TODO: how its possible for num_of_ack to be 0 here? (look at line 105)
						if (Ack_Packet_Handler(&User_address, get_len_client, sock, (num_of_ack == 0 ? 0 : num_of_ack - 1)) == ERROR) {
							printf("FLOWERROR: failure send ACK %d, after timeout occured\n", (num_of_ack - 1));
							close(dest_file);
							Error_flag = true;
							break;
						}

						if (timeoutExpiredCount >= NUMBER_OF_FAILURES) {
							printf("FLOWERROR: %d failures occured, exit...\n", NUMBER_OF_FAILURES);
							close(dest_file);
							Error_flag = true;
							break;
						}
					}
					else {  //return value of select is < 0
						perror("TTFTP_ERROR: select failure\n");
						close(dest_file);
						Error_flag = true;
						break;
					}
				} while (!Error_flag);
			} while (false);

			timeoutExpiredCount = 0;

			if (!Error_flag && is_packet_done == DONE) {
				if (Ack_Packet_Handler(&User_address, get_len_client, sock, num_of_ack) == ERROR) {
					fprintf(stderr, "TTFTP_ERROR: failure send ACK %d, and last\n", num_of_ack);
					perror("");
					Error_flag = true;
				}
				
				else
					printf("RECVOK\n");
				close(dest_file);
				break;
			}

		} while (!Error_flag);
	}

	close(sock);
	printf("RECVFAIL\n");
	return ERROR;
}