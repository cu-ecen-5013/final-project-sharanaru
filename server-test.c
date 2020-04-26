#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <string.h> 
#include <sys/socket.h> 
#define PORT 9000
#define SA struct sockaddr 


int main(int argc, char *argv[])
{
	int tester_socket;
	struct sockaddr_in servaddr;
	tester_socket = socket(AF_INET, SOCK_STREAM, 0); 
	setsockopt(tester_socket,SOL_SOCKET,SO_REUSEADDR, &(int){1},sizeof(int));
    if ( tester_socket== -1) 
    { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = inet_addr(argv[1]); 
    servaddr.sin_port = htons(PORT);   
    // connect the client socket to server socket 
   if(connect(tester_socket, (SA*)&servaddr, sizeof(servaddr)) == 0)
{
	printf("Server connect successfull");
    return 0;
}
printf("no");
    return 1;
}
