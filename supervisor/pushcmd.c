/*
	Usage: push_cmd VM_Name CMD_instruction

	Author: Ecular Xu
	<ecular_xu@trendmicro.com.cn>

	Author: Maxul Lee
	<maxul@bupt.edu.cn>

	Last Update: 2017/1/11
	
	cal_num() -> ftok()
	2017/2/4 MAX LEE

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define DBGExit(fmt, ...) { fprintf(stderr, fmt, ##__VA_ARGS__); exit(EXIT_FAILURE); }

int main(int argc, char **argv)
{
    int server_fd;
    int port_num;
    struct sockaddr_in server_add;
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s image cmd\n", argv[0]);
        return 1;
    }

    port_num = (int)ftok(argv[1], ~0);

    /* Using TCP to communicate with VM */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == server_fd)
        DBGExit("socket fail !\n");

    bzero(&server_add,sizeof(struct sockaddr_in));
    server_add.sin_family = AF_INET;
    server_add.sin_port = htons(port_num);
    server_add.sin_addr.s_addr = inet_addr("127.0.0.1");

    //printf("port number: %x\n", port_num);
    if (-1 == connect(server_fd, (struct sockaddr *)(&server_add), sizeof(struct sockaddr)))
        DBGExit("connect fail !\n");

    if(-1 == write(server_fd, argv[2], strlen(argv[2])))
        DBGExit("write fail !\n");

    close(server_fd);
    return 0;
}

