//Program to set up a socket server at port 9000
//referenced some functionalities from http://www.linuxhowtos.org/C_C++/socket.htm
//referenced http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html for daemon creation
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <time.h>
#include <arpa/inet.h>
#include <errno.h>
#include<syslog.h>
#include <signal.h>
#include <mqueue.h> 
#include <semaphore.h>
#include <stdint.h>
#include <pthread.h>
#define PORTNO 9000
#define USE_AESD_CHAR_DEVICE 1
volatile int flag_sysexit=1;
#define SNDRCV_MQ "/sensor_data"
int socketfd;//socket parameters
//msq queue parameters
#define QUEUE_NAME   "/mqueue"
#define SEM_MUTEX_NAME "/sem-mutex"
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_MSG_SIZE 256
#define MSG_BUFFER_SIZE MAX_MSG_SIZE + 10

sem_t *mutex_sem;

int broadcast_flag=1;
int recv_socket;
//exit function

void exit_cleanup()
{
    if(!flag_sysexit){
        shutdown(socketfd,SHUT_RDWR);
        //printf("Exiting\n");
        closelog();
        exit(0);
    }

}

void handle_sig(int sig)
{
    flag_sysexit=0;
    if(sig == SIGINT)
        syslog(LOG_DEBUG,"Caught SIGINT Signal exiting\n");
    if(sig == SIGTERM)
        syslog(LOG_DEBUG,"Caught SIGTERM Signal exiting\n");
    
    shutdown(socketfd,SHUT_RDWR);

    
}
// creating daemon process
void get_broadcast(const char * ip_address,char * broadcast_adress)
{  
    int no_dots=0;
    for(int i=0;i<strlen(ip_address);i++)
    {
        if(ip_address[i] == '.')
        {
            no_dots++;
            
            if(no_dots == 3)
                {   broadcast_adress[i] = ip_address[i];
                    broadcast_adress[++i]='2';
                    broadcast_adress[++i]='5';
                    broadcast_adress[++i]='5';
                    broadcast_adress[++i]='\0';
                    break;
                }
            }
            broadcast_adress[i] = ip_address[i];


    }
    printf("Broadcast address is %s\n",broadcast_adress);
}

char * runtime_input;
    
void *broadcast_server_address(void *arg)
{
    printf("In thread\n" );
    int broadcast_sock;                         /* Socket */
    struct sockaddr_in broadcastAddr; /* Broadcast address */
    char broadcastIP[30];                /* IP broadcast address */
    int broadcastPort;     /* Server port */
    char *sendString;                 /* String to broadcast */
    int broadcastPermission;          /* Socket opt to set permission to broadcast */
    unsigned int sendStringLen;       /* Length of string to broadcast */
    get_broadcast(runtime_input,broadcastIP);          /* First arg:  broadcast IP address */ 
    broadcastPort = 9010;    /* Second arg:  broadcast port */
    sendString = runtime_input;             /* Third arg:  string to broadcast - SERVER iP */

    /* Create socket for sending/receiving datagrams */
    if ((broadcast_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        perror("socket() failed");
    }

    /* Set socket to allow broadcast */
    broadcastPermission = 1;
    if (setsockopt(broadcast_sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission,sizeof(broadcastPermission)) < 0){
        perror("setsockopt() failed");
    }


    /* Construct local address structure */
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = inet_addr(broadcastIP);/* Broadcast IP address */
    broadcastAddr.sin_port = htons(broadcastPort);         /* Broadcast port */
    sendStringLen = strlen(sendString);  /* Find length of sendString */
    

    while(broadcast_flag) /* Run till connection is accepted */
    {
         /* Broadcast sendString in datagram to clients every 3 seconds*/
        if (sendto(broadcast_sock, sendString, sendStringLen, 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) != sendStringLen) 
             perror("sendto() sent a different number of bytes than expected");

        
     }
     //printf("Out of thread\n");
     return NULL;
    /* NOT REACHED */
 }

 void create_daemon()
 {

    pid_t process_id;
    process_id=fork();
    if (process_id < 0){
        syslog(LOG_DEBUG,"Fork failed\n");
        exit(1);
    }
    if(process_id > 0)
        exit(0);
    umask(0);
    if (setsid() < 0)
    {
        syslog(LOG_DEBUG,"Child process does not lead\n");
        exit(1);
    }
    if(chdir("/") < 0)
    {
        syslog(LOG_DEBUG,"Directory change filed \n"); // chdir() failed.
        exit(1);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}



int main(int argc, char *argv[])
{   
    //signal handlers redirection
    signal(SIGTERM,handle_sig);
    signal(SIGINT,handle_sig);
    openlog(NULL,LOG_NDELAY,LOG_USER);
    runtime_input=argv[1];
    
    if(argc == 3)
    {
        create_daemon();
    }
    //setting up broadcast thread
    pthread_t broadcast_thread;
    pthread_create(&broadcast_thread,NULL,broadcast_server_address,NULL);
    
    //setting up socket
    socklen_t  clientsize;
    struct sockaddr_in server,client;
    server.sin_family=AF_INET;
    server.sin_port=htons(PORTNO);
    server.sin_addr.s_addr=INADDR_ANY;
    socketfd=socket(AF_INET,SOCK_STREAM,0);
    setsockopt(socketfd,SOL_SOCKET,SO_REUSEADDR, &(int){1},sizeof(int));
    clientsize=sizeof(client);

    if(socketfd == -1)
    {
        //printf("\nSocket setup failed \n");
        return -1;
    }

    if(bind(socketfd,(struct sockaddr *) &server, sizeof(server)) ==-1)
    {
        syslog(LOG_ERR,"\nBinding failed \n");
        perror("Binding fail:");
        return -1;
    }

    if(listen(socketfd,100) == -1)
    {
        syslog(LOG_ERR,"Listen failed. Socket not available");
    }

    bzero((char *) &server, sizeof(server));
    printf("/n Waiting to accept\n");
    recv_socket=accept(socketfd,(struct sockaddr *)&client, &clientsize);
    broadcast_flag=0;
    syslog(LOG_DEBUG,"Accepted connection from %s\n",inet_ntoa(client.sin_addr));
    printf("Accepted connection from %s\n",inet_ntoa(client.sin_addr));
    pthread_join(broadcast_thread,NULL);
    //setting up queue
    mqd_t qd_tx;
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_MSG_SIZE;
    attr.mq_curmsgs = 0;

    if ((qd_tx = mq_open (QUEUE_NAME, O_WRONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror ("Server: mq_open (server)");
        exit(-1);
    }

    //opening named semaphore
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED) {
        perror ("sem_open"); exit (1);
    }

    
    const char send_msg[]="Send\n";

    while(flag_sysexit)
    {
        char recv_message[40]={0};
        int read_status=0;
        if (sem_wait (mutex_sem) == -1) 
        {
           perror ("sem_take Error"); 
           exit (1);
        }
	   
       //asking for new data from sensor fusion device
       write(recv_socket,send_msg,strlen(send_msg));
       //using \n as terminating character

       while(recv_message[read_status] != '\n')
       {
        read_status+=read(recv_socket,recv_message+read_status,10);
       }
 	//load string to msg_q
       recv_message[read_status]='\0';

       if (mq_send (qd_tx, recv_message, strlen(recv_message)+1, 0)   == -1) 
       {
            perror ("Server: Not able to send message to client");   
       }

       memset(recv_message,0,40);   
    }
    //printf("Out of loop");
    return 0;
}
