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
#include <stdint.h>

#define PORTNO 9000
#define USE_AESD_CHAR_DEVICE 1
volatile int flag_sysexit=1;
#define SNDRCV_MQ "/sensor_data"
int socketfd;//socket parameters
//msq queue parameters
static mqd_t mymq;
struct mq_attr mq_attr;

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
    if(sig == SIGINT)
        syslog(LOG_DEBUG,"Caught SIGINT Signal exiting\n");
    if(sig == SIGTERM)
        syslog(LOG_DEBUG,"Caught SIGTERM Signal exiting\n");
    flag_sysexit=0;
    shutdown(socketfd,SHUT_RDWR);

    
}
// creating daemon process

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

int queue_init()
{
    mq_attr.mq_maxmsg = 1;
    mq_attr.mq_msgsize = 10 * sizeof(char); //temporary length
    mq_attr.mq_flags = 0;     
    mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, 0, &mq_attr);
    if(mymq == (mqd_t)-1){
        return -1;
    }
    return 0;
}

 

int main(int argc, char *argv[])
{
    //signal handlers redirection
    signal(SIGTERM,handle_sig);
    signal(SIGINT,handle_sig);
    openlog(NULL,LOG_NDELAY,LOG_USER);
    if(argc == 2)
    {
        create_daemon();
    }
    //setting up queue
    if(queue_init() != 0){
        syslog(LOG_ERR,"Queue setup failed\n");
    }

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
    if(bind(socketfd,(struct sockaddr *) &server, sizeof(server)) == -1)
    {
        syslog(LOG_ERR,"\nBinding failed \n");
        return -1;
    }

    if(listen(socketfd,100) == -1)
    {
        syslog(LOG_ERR,"Listen failed. Socket not available");
    }

    bzero((char *) &server, sizeof(server));
    int recv_socket=accept(socketfd,(struct sockaddr *)&client, &clientsize);
    syslog(LOG_DEBUG,"Accepted connection from %s\n",inet_ntoa(client.sin_addr));
    while(flag_sysexit)
    {
        
        if(recv_socket < 0)
            syslog(LOG_ERR,"Failure in accept\n");
        char recv_message[30]={0};
        recv_message[20]='\0';
        int read_status;
        read_status=read(recv_socket,recv_message,100);
	    syslog(LOG_DEBUG,"Read %d bytes\n",read_status);
        // mq_attr get_queue_params;
        // mq_getattr(mymq,&get_queue_params);
        // if(get_queue_params.mq_curmsgs == 0)
        // {
        // mq_send(mymq, recv_message, strlen(recv_message) , 30);
        // }

        if(strcmp(recv_message,"Close Server") == 0)
        {
            
            syslog(LOG_DEBUG,"Received Server close message\n");
            flag_sysexit=0;
            handle_sig(SIGTERM);
        }
        
     syslog(LOG_DEBUG,"%s\n",recv_message);
     memset(recv_message,0,30);     
    }
    
    
    return 0;

}
