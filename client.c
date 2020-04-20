#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <time.h>
#include <arpa/inet.h>
#include <errno.h>
#include<syslog.h>
#include <signal.h>
#include <stdint.h>

#define PORTNO 9000

int main(int argc, char *argv[])
{
  int socket_client;
  struct sockaddr_in server_details;
  socklen_t addr_size;
  socket_client = socket(AF_INET, SOCK_STREAM, 0);
  server_details.sin_family = AF_INET; 
  server_details.sin_port = htons(PORTNO);
  server_details.sin_addr.s_addr = inet_addr(argv[1]);
  printf("Server at %s\n",argv[1]);
  memset(server_details.sin_zero, '\0', sizeof (server_details.sin_zero));
  addr_size = sizeof(server_details);
  if(connect(socket_client, (struct sockaddr *) &server_details, addr_size) == -1){
    printf("Connect to server failed\n");
  }
  char message[10]; 
  strcpy(message,"Hello");
  if(send(socket_client , message , strlen(message) , 0) < 0)
    {
        printf("Send failed\n");

    }
  shutdown(socket_client,SHUT_RDWR);
    //Read the message from the server into the buffer
    //Print the received message
}