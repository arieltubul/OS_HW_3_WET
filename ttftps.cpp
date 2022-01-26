#include "ttftps.h"

int main(int argc, char** argv){
    //argv[0]=name of program, argv[1]=portNum, argv[2]=timout, argv[3]=failures_num
    // input check
//    cout<<"A"<<endl;
    if((argc != 4) || !( (MIN_PORT_NUM < atoi(argv[1])) && (atoi(argv[1]) <= MAX_PORT_NUM) )  || (atoi(argv[2])<=0) || (atoi(argv[3])<=0)) {
        cout << "TTFTP_ERROR: illegal arguments" << endl;
        exit(ERROR);
    }
//    cout<<"B"<<endl;
    uint16_t serv_port = atoi(argv[1]);
    uint16_t time_out = atoi(argv[2]);
    uint16_t failures_num = atoi(argv[3]);

    /*setting server socket*/
    sock_addr_in serv_addr;
    //memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(serv_port);

    char buffer[PACKET_MAX_SIZE];
    int sock = socket(AF_INET,SOCK_DGRAM, 0);
    if(sock == ERROR)
        perror_func();


    /*binding the socket*/
    if (bind(sock, (p_sock_addr)&serv_addr, sizeof(serv_addr)) < 0)
        perror_func();

    All_clients clients; //map of clients.clientList sorted wrt time of last packet sending
    fd_set read_fds;
    timeval select_timeout;
    int i=-1;
    while(true) //running in endless loop
    {
        i++;
        cout<<"C"<<i<<endl;
        /*initializing read_fds*/
        FD_ZERO(&read_fds); //clears the read_fds
        FD_SET(sock, &read_fds); //adds file descriptor to the set
        /*initializing timout*/
        cout<<i<<": map size is: "<<clients.clientList.size()<<endl;
        select_timeout.tv_usec = 0; //milli seconds
        if(!clients.clientList.size()) {
            cout<<"im here 1"<<endl;
            // no clients.clientList in map
            select_timeout.tv_sec = time_t(time_out);
        }
        else
        {
            cout<<"im here 2"<<endl;
            map<int, Client*>::iterator timeout_client = clients.get_earliest_packet_client();
            time_t cur_time = time(NULL);
            double diff = difftime(cur_time, timeout_client->second->last_packet_time);
            select_timeout.tv_sec = time_out - diff; //diff(end, begin)
        }
        cout<<"D"<<i<<endl;

        int select_result = select((sock + 1), &read_fds, NULL, NULL, &select_timeout);
        cout<<"E"<<i<<endl;

        /*FIRST CASE: we got a client that wants to send a packet*/
        if (FD_ISSET(sock, &read_fds) && select_result > 0)
        {
            cout<<"F"<<i<<endl;

            sock_addr_in tmp_client_addr;
            socklen_t get_len_client = sizeof(tmp_client_addr);
            int read_packet_size = recvfrom(sock, buffer, PACKET_MAX_SIZE, 0, (p_sock_addr)&tmp_client_addr, &get_len_client);
            if(read_packet_size < 0) //recvfrom failed
                perror_func();

            char* fname = strdup(buffer + OP_BLOCK_FIELD_SIZE); //need to free in client C'tor
            cout<<"filename in begining is: "<<fname<<endl;

            /*handling opcode*/
            uint16_t tmp_opcode;
            memcpy(&tmp_opcode, buffer, OP_BLOCK_FIELD_SIZE);
            tmp_opcode = ntohs(tmp_opcode);

//            /*handling ip and port*/
//            uint16_t tmp_port = ntohs(tmp_client_addr.sin_port);
//            char* tmp_ip= inet_ntoa(tmp_client_addr.sin_addr);
            cout<<"G"<<i<<endl;
            map<int, Client*>::iterator tmp_client = clients.get_client(tmp_client_addr);
            cout<<"size of clients map before check errors in data case is: "<<clients.clientList.size()<<endl;
            if(tmp_client != clients.clientList.end()) //it means the client is already in map, so we expect to data packet
            {
                /* error #2 */
                if((read_packet_size < PACKET_HEADER_SIZE) || (tmp_opcode == ACK_OP)) //TODO: make sure that we can't get here an ACK packet by a mistake
                {
                    close(tmp_client->second->fd);
                    remove(tmp_client->second->file_name);
                    send_error_msg(4, "Illegal TFTP operation", &tmp_client_addr, sizeof(tmp_client_addr), sock);
                    delete(tmp_client->second);
                    clients.clientList.erase(tmp_client);
                    continue;
                }

                /* error #3 */
                if(tmp_opcode == WRQ_OP)
                {
                    close(tmp_client->second->fd);
                    remove(tmp_client->second->file_name);
                    send_error_msg(4, "Unexpected packet", &tmp_client_addr, sizeof(tmp_client_addr), sock);
                    delete(tmp_client->second);
                    clients.clientList.erase(tmp_client);
                    continue;
                }

                //here we know we got a data packet(opcode = DATA_OP), so we check if it's valid

                /*handling block_num*/
                uint16_t tmp_block_num;
                memcpy(&tmp_block_num, buffer+OP_BLOCK_FIELD_SIZE, OP_BLOCK_FIELD_SIZE);
                tmp_block_num = ntohs(tmp_block_num);

                /* error #6 */
                if(tmp_block_num != tmp_client->second->last_block_num + 1)
                {
                    close(tmp_client->second->fd);
                    remove(tmp_client->second->file_name);
                    send_error_msg(0, "Bad block number", &tmp_client_addr, sizeof(tmp_client_addr), sock);
                    delete(tmp_client->second);
                    clients.clientList.erase(tmp_client);
                    continue;
                }

                /*here we know we got a valid data packet*/
                /*writing to client file*/
                int read_data_size = read_packet_size - PACKET_HEADER_SIZE;
                cout<<"data opcode: "<<tmp_opcode<<endl;
                cout<<"data blocknum: "<<tmp_block_num<<endl;
                cout<<"real data: "<<(buffer+4)<<endl;
                if(write(tmp_client->second->fd, buffer+PACKET_HEADER_SIZE, read_data_size)<read_data_size)
                    perror_func();


                cout<<"map size check BEFORE UPDATE: "<<clients.clientList.size()<<endl;
                if(read_packet_size < DATA_PACKET_MAX_SIZE) //transaction process finished successfully
                {
                    close(tmp_client->second->fd);
                    delete(tmp_client->second);
                    clients.clientList.erase(tmp_client); //cause we finished with this client
                    cout<<"it was the last data packet"<<endl;
                }
                else //there are more data packets and transaction process finished successfully
                {
                    tmp_client->second->last_block_num = tmp_block_num;
                    tmp_client->second->last_packet_time = time(NULL); //updating time of last packet
                    cout<<"map size check IN UPDATE: "<<clients.clientList.size()<<endl;
                    cout<<"TIME IN UPDATE: "<<clients.clientList.begin()->second->last_packet_time<<endl;

//                     clients.clientList.erase(tmp_client);
                }
                cout<<"map size check AFTER UPDATE: "<<clients.clientList.size()<<endl;
                /*sending ack packet back to client. relevant whether it was the last dtata packet or not*/
                send_ack(&tmp_client_addr, sizeof(tmp_client_addr), sock, tmp_block_num);
            }//if(tmp_client)


            else//it means the client is NOT in map, so we expect to get WRQ packet
            {
                cout<<"wrq_p"<<i<<endl;

                /* error #1 */
                if(tmp_opcode == DATA_OP)
                {
                    send_error_msg(7, "Unknown user", &tmp_client_addr, sizeof(tmp_client_addr), sock);
                    cout<<"wrq_p"<<i<<" error1"<<endl;
                    continue;
                }

                /* error #2 */
                if((read_packet_size < OP_BLOCK_FIELD_SIZE) || (tmp_opcode != WRQ_OP))
                {
                    cout<<"wrq_p"<<i<<" error2"<<endl;
                    send_error_msg(4, "Illegal TFTP operation", &tmp_client_addr, sizeof(tmp_client_addr), sock);
                    continue;
                }

                /* error #4 */
                char packet[DATA_PACKET_MAX_SIZE];
                memcpy(packet, buffer, sizeof(buffer));
                if(check_WRQ_msg(packet) == false)
                {
                    cout<<"wrq_p"<<i<<" error4"<<endl;
                    send_error_msg(4, "Illegal WRQ", &tmp_client_addr, sizeof(tmp_client_addr), sock);
                    cout<<"send error successfully"<<endl;
                    continue;
                }

//                /* error #5 */
//                char* filename = strdup(buffer + OP_BLOCK_FIELD_SIZE);
//                if(clients.file_exists(filename) == true)
//                {
//                    cout<<"wrq_p"<<i<<" error5"<<endl;
//                    send_error_msg(6, "File already exists", &tmp_client_addr, sizeof(tmp_client_addr), sock);
////                    free(filename); //strdup allocates memory
//                    continue;
//                }
////                free(filename); //strdup allocates memory

                /*here we know we got a valid data packet*/
                cout<<"about to add a new client"<<endl;
                char* filename = strdup(buffer + OP_BLOCK_FIELD_SIZE); //need to free in client C'tor
                cout<<"filename in cpp is: "<<filename<<endl;
                clients.add_new_client(buffer, tmp_client_addr, sock);
                cout<<"I"<<i<<endl;
                send_ack(&tmp_client_addr, sizeof(tmp_client_addr), sock, 0);
                cout<<"earlist: filename: "<<tmp_client->second->file_name<<", last time: "<<tmp_client->second->last_packet_time<<endl;
            }
        }


            /*SECOND CASE: timeout*/
        else if(select_result ==0)
        {
            cout<<"timout"<<endl;
            //check if the map is empty so it wasn't a real timeout
            if (clients.clientList.size() == 0)
                continue;

            map<int, Client*>::iterator bad_client = clients.get_earliest_packet_client();
            cout<<"earlist: filename: "<<bad_client->second->file_name<<", last time: "<<bad_client->second->last_packet_time<<endl;
            //TODO:DEBUG
            map<int, Client*>::iterator it = clients.clientList.begin();
//            for (; it != clients.clientList.end(); ) //erase automatically promotes it to next node
//            {
//                cout<<"printing file names and last time of packet to every alive client:"<<endl;
//                cout<<"file name: "<<it->second->file_name<<", last time: "<<it->second->last_packet_time<<endl;
//            }
            //till here debug
            sock_addr_in user_addr =  bad_client->second->client_address;
            socklen_t len = sizeof(user_addr);

            bad_client->second->fails_num++;
            //handling the case that this client did too many failures
            if (bad_client->second->fails_num > failures_num)
            {
                close(bad_client->second->fd);
                remove(bad_client->second->file_name);
                send_error_msg(0, "Abandoning file transmission", &user_addr, len, sock);
                delete(bad_client->second);
                clients.clientList.erase(bad_client);
            }

                //the client didn't pass failure max
            else
            {
                //sending recent Ack message to client and the system call didn't succeed
                send_ack(&user_addr, len, sock, bad_client->second->last_block_num);
            }
        }


            /*THIRD CASE: select sys call failed*/
        else if (select_result<0)
        {
            cout<<"select failed"<<endl;
            perror_func();
        }
        cout<<i<<": last check map size: "<<clients.clientList.size()<<endl;

    }//while(true)
    close(sock);
    return ERROR;
}