#include "ttftps.h"

int main(int argc, char** argv)
{
    //argv[0]=name of program, argv[1]=portNum, argv[2]=timout, argv[3]=failures_num
    // input check
    if((argc != 4) || !(MIN_PORT_NUM < atoi(argv[1]) <= MAX_PORT_NUM)  || (atoi(argv[2])<=0) || (atoi(argv[3])<=0)) {
        cout << "TTFTP_ERROR: illegal arguments" << endl;
        exit(ERROR);
    }

    uint16_t serv_port = atoi(argv[1]);
    uint16_t time_out = atoi(argv[2]);
    uint16_t failures_num = atoi(argv[3]);

    /*setting server socket*/
    sock_addr_in serv_addr;
    //memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(serv_port);

    char buffer[DATA_PACKET_MAX_SIZE];
    int sock = socket(AF_INET,SOCK_DGRAM, 0);
    if(sock == ERROR)
        perror_func();


    /*binding the socket*/
    if (bind(sock, (p_sock_addr)&serv_addr, sizeof(serv_addr)) < 0)
        perror_func();
    //FIXME: overloading for comparison operator <
    Clients_map clients; //map of clients sorted wrt time of last packet sending
    fd_set read_fds;
    timeval select_timeout;

    while(true) //running in endless loop
    {
        /*initializing read_fds*/
        FD_ZERO(&read_fds); //clears the read_fds
        FD_SET(sock, &read_fds); //adds file descriptor to the set

        /*initializing timout*/
        select_timout.tv_usec = 0; //milli seconds
        if(!clients) // no clients in map
            select_timout.tv_sec = time_t(time_out);
        else
        {
            time_t cur_time;
            time(&cur_time);
            select_timout.tv_sec = time_t(time_out - difftime(cur_time, (clients)[0]->first)); //diff(end, begin)
        }

        select_result = select((sock + 1), &read_fds, NULL, NULL, &select_timeout);

        /*FIRST CASE: we got a client that wants to send a packet*/
        if (FD_ISSET(sock, &read_fds) && select_result > 0)
        {
            sock_addr_in tmp_client_addr;
            int read_packet_size = recvfrom(sock, buffer, PACKET_MAX_SIZE, 0, (p_sock_addr)&tmp_client_addr, sizeof(tmp_client_addr));
            if(read_packet_size < 0) //recvfrom failed
                perror_func();

            /*handling opcode*/
            uint16_t tmp_opcode;
            memcpy(&tmp_opcode, buffer, OP_BLOCK_FIELD_SIZE);
            tmp_opcode = ntohs(tmp_opcode);

//            /*handling ip and port*/
//            uint16_t tmp_port = ntohs(tmp_client_addr.sin_port);
//            char* tmp_ip= inet_ntoa(tmp_client_addr.sin_addr);

            Client* tmp_client = get_client(tmp_client_addr); //FIXME: GET CLIENT gets address in struct
            if(tmp_client) //it means the client is already in map, so we expect to data packet
            {
                /* error #2 */
                if((read_packet_size < PACKET_HEADER_SIZE) || (tmp_opcode == ACK_OP)) //TODO: make sure that we can't get here an ACK packet by a mistake
                {
                    send_error_msg(4, "Illegal TFTP operation");
                    erase(tmp_client);
                    continue;
                }

                /* error #3 */
                if(tmp_opcode == WRQ_OP)
                {
                    send_error_msg(4, "Unexpected packet");
                    erase(tmp_client);
                    continue;
                }

                //here we know we got a data packet(opcode = DATA_OP), so we check if it's valid

                /*handling block_num*/
                uint16_t tmp_block_num;
                memcpy(&tmp_block_num, buffer+OP_BLOCK_FIELD_SIZE, OP_BLOCK_FIELD_SIZE);
                tmp_block_num = ntohs(tmp_block_num);

                /* error #6 */
                if(tmp_block_num != tmp_client->second.last_block_num + 1)
                {
                    send_error_msg(0, "Bad block number");
                    erase(tmp_client);
                    continue;
                }

                /*here we know we got a valid data packet*/
                /*writing to client file*/
                int read_data_size = read_packet_size - PACKET_HEADER_SIZE;
                if(write(tmp_client->fd, buffer+PACKET_HEADER_SIZE, read_data_size)<read_data_size)
                    perror_func();

                if(read_packet_size == DATA_PACKET_MAX_SIZE) //there are more data packets and transaction process finished successfully
                {
                     tmp_client->second.last_block_num = tmp_block_num;
                     //updating time by inserting a new updated client and erase the previous one
                     time_t current_time;
                     time(&current_time);
                     clients[current_time] = tmp_client->second; //FIXME: we need to implement copy C'tor
                     erase(tmp_client);
                }
                /*sending ack packet back to client. relevant whether it was the last dtata packet or not*/
                send_ack(&tmp_client_addr, sizeof(tmp_client_addr), sock, tmp_block_num);
            }//if(tmp_client)


            else//it means the client is NOT in map, so we expect to get WRQ packet
            {
                /* error #1 */
                if(tmp_opcode == DATA_OP)
                {
                    send_error_msg(7, "Unknown user");
                    continue;
                }

                /* error #2 */
                if((read_packet_size < OP_BLOCK_FIELD_SIZE) || (tmp_opcode != WRQ_OP))
                {
                    send_error_msg(4, "Illegal TFTP operation");
                    continue;
                }

                /* error #4 */
                char packet[DATA_PACKET_MAX_SIZE];
                memcpy(packet, buffer, sizeof(buffer));
                if(is_wrq_packet_valid(packet) == false)
                {
                    send_error_msg(4, "Illegal WRQ");
                    continue;
                }

                /* error #5 */
                char* filename = strdup(buffer + OP_BLOCK_FIELD_SIZE);
                if(FileNameExists(filename))
                {
                    send_error_msg(6, "File already exists");
                    continue;
                }

                /*here we know we got a valid data packet*/
                create_client_from_WRQpacket(buffer);
                send_ack(&tmp_client_addr, sizeof(tmp_client_addr), sock, 0);

            }
		//handling the case that we had timeout	
        }else if (select_result ==0){
			//check if the map is empty
			if ((clients->clientList).size() == 0){
				continue;
			}
			Client *first_client=((clients->clientList).begin())->second;
			first_client->fails_num++;
			//handling the case that this client did too many failures
			if (first_client->fails_num > failures_num){
				//sending error message to client and the system call didn't succeed
				if (send_error_msg(0, "Abandoning file transmission",first_client->client_address,
				sizeof(first_client->client_address),sock)!= 0){
					//FIXME: to check if we need to delete all files also
					delete clients;
					close (sock);
					perror_func();
				//removing client from map
				}else{
					clients.deleteClient(first_client->client_address);
				}
			//the client didn't pass failiure max
			}else {
				//sending recent Ack message to client and the system call didn't succeed
				if (SendAck(first_client->client_address, sizeof(first_client->client_address),sock,
                    first_client->block_num)!= 0){
					//FIXME: to check if we need to delete all files also
					delete clients;
					close (sock);
					perror_func();
				}
			}
		//handling the case the select system call failed
        } else if (sel_result<0){
			//FIXME: to check if we need to delete all files also
			delete clients;
			close (sock);
			perror_func();
		}
    }
	return ERROR;
}
