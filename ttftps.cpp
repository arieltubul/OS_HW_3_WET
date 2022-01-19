#include "ttftps.h"

int main(int argc, char** argv)
{
    //argv[0]=name of program, argv[1]=portNum, argv[2]=timout, argv[3]=failures_num
    // input check
    if((argc != 4) || !(MIN_PORT_NUM < atoi(argv[1]) <= MAX_PORT_NUM)  || (atoi(argv[2])<=0) || (atoi(argv[3])<=0)) {
        cout << "TTFTP_ERROR: illegal arguments" << endl;
        exit(ERROR);
    }

    unsigned short serv_port = atoi(argv[1]);
    unsigned short time_out = atoi(argv[2]);
    unsigned short failures_num = atoi(argv[3]);

    //setting server socket
    p_sock_addr_in serv_addr={nullptr};
    //memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr->sin_port = htons(serv_port);

    char buffer[PACKET_MAX_SIZE];
    int sock = socket(AF_INET,SOCK_DGRAM, 0);
    if(sock == ERROR)
        perror_func();


    // binding the socket
    if (bind(sock, (p_sock_addr) &serv_addr, (socklen_t)sizeof(serv_addr)) < 0)	{
        close(reinterpret_cast<FILE *>(sock)); //FIXME: casting
        perror_func();
    }
    Clients clients;
    while(true) //running in endless loop
    {
        fd_set read_fds;
        FD_ZERO(&read_fds); //clears the read_fds
        FD_SET(sock, &read_fds); //adds file descriptor to the set

        time select_timeout;
        select_timout.tv_usec = 0;

        if(!clients)
            select_timout.tv_sec = time_out;
        else
        {
            time_t cur_time;
            time(&cur_time);
            select_timout.tv_sec = time_out - difftime(sort(clients)[0]->first, cur_time);
        }

        select_result = select((sock + 1), &read_fds, NULL, NULL, &select_timeout);

        //first case: we got a client that wants to send packet
        if (FD_ISSET(sock, &read_fds) && select_result > 0) {
            p_sock_addr tmp_client;
            int data_size = recvfrom(sock, buffer, PACKET_MAX_SIZE, 0, (p_sock_addr)&tmp_client, sizeof(p_sock_addr));
        }
    }















    return ERROR;
}