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
#include "MadgwickAHRS.h"
#include<syslog.h>
#include <signal.h>
#include <stdint.h>
#define MAXRECVSTRING 30
#define PORTNO 9000
int operation_switch =1;

void handle_sig(int sig)
{
    operation_switch=0;
    if(sig == SIGINT)
        syslog(LOG_DEBUG,"Caught SIGINT Signal exiting\n");
    if(sig == SIGTERM)
        syslog(LOG_DEBUG,"Caught SIGTERM Signal exiting\n");  
}

int main(int argc, char *argv[])
{
    signal(SIGTERM,handle_sig);
    signal(SIGINT,handle_sig);
  //Inital receive broadcast message - Setting up socket with broadcast permisson to receive
    int sock;                         /* Socket */
    struct sockaddr_in broadcastAddr; /* Broadcast Address */
    unsigned int broadcastPort;     /* Port */
    char recvString[MAXRECVSTRING+1]; /* Buffer for received string */
    int recvStringLen;                /* Length of received string */
    broadcastPort = 9010;   /* First arg: broadcast port */
    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        perror("socket() failed");
    /* Construct bind structure */

    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
    broadcastAddr.sin_port = htons(broadcastPort);      /* Broadcast port */

    /* Bind to the broadcast port */
    if (bind(sock, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) < 0)
        perror("bind() failed");

    /* Receive a single datagram from the server */
    if ((recvStringLen = recvfrom(sock, recvString, MAXRECVSTRING, 0, NULL, 0)) < 0)
        perror("recvfrom() failed");

    recvString[recvStringLen] = '\0';
    //printf("Received: %s\n", recvString);    /* Print the received string */
	  //Closing socket
    shutdown(sock,SHUT_RDWR);
    
    //setting up TCP socket

    int socket_client;
	  socket_client=socket(PF_INET,  SOCK_STREAM, 0);
	  if(socket_client <0)
	   perror("Socket setup");
  	struct sockaddr_in server_details;   
    socklen_t addr_size;
  	socket_client = socket(AF_INET, SOCK_STREAM, 0);
  	server_details.sin_family = AF_INET; 
   	server_details.sin_port = htons(PORTNO);
   	server_details.sin_addr.s_addr = inet_addr(recvString);
   	memset(server_details.sin_zero, '0', sizeof (server_details.sin_zero));
   	addr_size = sizeof(server_details);
   	
   	if(connect(socket_client, (struct sockaddr *) &server_details, addr_size) == -1)
   {
     printf("Connect to server failed\n");
   }
   while(operation_switch)
  {
   char send_cmd[10]={0}; 
   int read_status=0;
   while(send_cmd[read_status] != '\n'){
    read_status=read(socket_client,send_cmd+read_status,10);
   }
   send_cmd[read_status]='\0';
   if(strcmp(send_cmd,"Send\n") == 0)
   {
      char * sensor_fusion_result = NULL;
      sensor_fusion_result= (char *) malloc(100);
      if(sensor_fusion_result == NULL)
      {
        syslog(LOG_DEBUG,"Malloc failed");
      }

      //Run sensor algorithm and then send sensor fusion commands
      
   }
 }
  //shutdown(socket_client,SHUT_RDWR);
    //Read the message from the server into the buffer
    //Print the received message
}
