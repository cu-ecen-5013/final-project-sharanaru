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
#include <math.h>
#include <arpa/inet.h>
#include <errno.h>
#include "MadgwickAHRS.h"
#include<syslog.h>
#include "i2c-dev.h"
#include <signal.h>
#include <stdint.h>

#define MAXRECVSTRING 30
#define PORTNO 9000

int operation_switch =1;
//UDP BROADCAST referred from http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/code/BroadcastReceiver.c
//TCP socket descriptor
int socket_client;

//Signal HAndler function
void handle_sig(int sig)
{
  operation_switch=0;
  if(sig == SIGINT)
    syslog(LOG_DEBUG,"Caught SIGINT Signal exiting\n");
  if(sig == SIGTERM)
    syslog(LOG_DEBUG,"Caught SIGTERM Signal exiting\n");  
  shutdown(socket_client,SHUT_RDWR);
  _exit(0);
}

/* Main routine initally sets up a a UDP receiver socket and waits to receive packet containing Server address. 
Once received it then sets up the i2c sensor device and then calls for the TCP socket to be setup and then tries connected to the obtained address. 
Messages from socket are continously read and compared with a predefined message which indicates fetching new sensor values and then sending the msg through the
TCp socket in the predefined format. 
*/
  
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
  
  if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
  {
    perror("socket() failed");
    /* Construct bind structure */
  }
    
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
  printf("Server at: %s\n", recvString);    /* Print the received string */
	  //Closing UDP socket
  shutdown(sock,SHUT_RDWR);

   //I2C init
   //setting up I2C bus
  int file_i2c, file_i2c_mag;
	//int length;
  uint32_t buffer[18] = {0};
  uint32_t ax =0, ay = 0, az = 0, gx = 0, gy = 0, gz = 0, mx = 0, my = 0, mz = 0;

	//----- OPEN THE I2C BUS -----
  char *filename = (char*)"/dev/i2c-1";
  if ((file_i2c = open(filename, O_RDWR)) < 0)
  {
		//ERROR HANDLING: you can check errno to see what went wrong
    printf("Failed to open the i2c bus");
    return -1;
  }
  if ((file_i2c_mag = open(filename, O_RDWR)) < 0)
  {
		//ERROR HANDLING: you can check errno to see what went wrong
    printf("Failed to open magnetometer  the i2c bus");
    return -1;
  }

	int addr = 0x68;          //<<<<<The I2C address of the slave
	int addr_mag = 0x0C; 
	if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
	{
		printf("Failed to acquire bus access and/or talk to slave.\n");
		//ERROR HANDLING; you can check errno to see what went wrong
		return -1 ;
	}
	if (ioctl(file_i2c_mag, I2C_SLAVE, addr_mag) < 0)
	{
		printf("Failed to acquire bus access for magnetometer and/or talk to slave.\n");
		//ERROR HANDLING; you can check errno to see what went wrong
		return -1 ;
	}
	i2c_smbus_write_byte_data(file_i2c, 0x37, 0x22);
  

  //setting up TCP socket
  
  socket_client=socket(PF_INET,  SOCK_STREAM, 0);
  if(socket_client <0)
    perror("Socket setup");
  
  //Socket parameters
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
    read_status=read(socket_client,send_cmd,10);
    send_cmd[read_status]='\0';

  if(strcmp(send_cmd,"Send\n") == 0)
  {
    char * sensor_fusion_result = NULL;
    sensor_fusion_result= (char *) malloc(100);
  
    if(sensor_fusion_result == NULL)
    {
      syslog(LOG_DEBUG,"Malloc failed");
    }
    //
    //Run sensor algorithm and then send sensor fusion commands
   
    buffer[0] = i2c_smbus_read_byte_data(file_i2c, 0x3B); // AccelerometerX High Byte
		buffer[1] = i2c_smbus_read_byte_data(file_i2c, 0x3C); // AccelerometerX Low Byte
		buffer[2] = i2c_smbus_read_byte_data(file_i2c, 0x3D); // AccelerometerY High Byte
		buffer[3] = i2c_smbus_read_byte_data(file_i2c, 0x3E); // AccelerometerY Low Byte
		buffer[4] = i2c_smbus_read_byte_data(file_i2c, 0x3F); // AccelerometerZ High Byte
		buffer[5] = i2c_smbus_read_byte_data(file_i2c, 0x40); // AccelerometerZ Low Byte

	 

		/* Reading gyroscope values */
		buffer[6] = i2c_smbus_read_byte_data(file_i2c, 0x43); // GyroscopeX High Byte
		buffer[7] = i2c_smbus_read_byte_data(file_i2c, 0x44); // GyroscopeX Low Byte
		buffer[8] = i2c_smbus_read_byte_data(file_i2c, 0x45); // GyroscopeY High Byte
		buffer[9] = i2c_smbus_read_byte_data(file_i2c, 0x46); // GyroscopeY Low Byte
		buffer[10] = i2c_smbus_read_byte_data(file_i2c, 0x47); // GyroscopeZ High Byte
		buffer[11] = i2c_smbus_read_byte_data(file_i2c, 0x48); // GyroscopeZ Low Byte
		
	
		
    i2c_smbus_write_byte_data(file_i2c_mag, 0x0A, (1 << 4) | 0x06);		
		/* Reading Magnetometer values */
		uint32_t magStatus1 = 0;
		uint32_t deviceID = i2c_smbus_read_byte_data( file_i2c_mag,0x00);
		printf("Device ID is %x\n",deviceID);
		do
    {
      magStatus1 = i2c_smbus_read_byte_data( file_i2c_mag,0x02);
    } while (!(magStatus1&0x0001));

		buffer[12] = i2c_smbus_read_byte_data(file_i2c_mag, 0x04); // MagnetometerX High Byte
		buffer[13] = i2c_smbus_read_byte_data(file_i2c_mag, 0x03); // MagnetometerX Low Byte
		buffer[14] = i2c_smbus_read_byte_data(file_i2c_mag, 0x06); // MagnetometerY High Byte
		buffer[15] = i2c_smbus_read_byte_data(file_i2c_mag, 0x05); // MagnetometerY Low Byte
		buffer[16] = i2c_smbus_read_byte_data(file_i2c_mag, 0x08); // MagnetometerZ High Byte
		buffer[17] = i2c_smbus_read_byte_data(file_i2c_mag, 0x07); // MagnetometerZ Low Byte
		
		printf("Magnetometer values are %d, %d, %d, %d, %d, %d\n", buffer[12], buffer[13], buffer[14], buffer[15], buffer[16], buffer[17]);
		/* Converting the accelerometer values into 16 bits */
		ax = buffer[0] << 8 | buffer[1];
		ay = buffer[2] << 8 | buffer[3];
		az = buffer[4] << 8 | buffer[5];
		/* Converting the gyroscope values into 16 bits */
		gx = buffer[6] << 8 | buffer[7];
		gy = buffer[8] << 8 | buffer[9];
		gz = buffer[10] << 8 | buffer[11];

		/* Converting the gyroscope values into 16 bits */
		mx = buffer[12] << 8 | buffer[13];
		my = buffer[14] << 8 | buffer[15];
		mz = buffer[16] << 8 | buffer[17];
		//printf("The magnetometer values are %d, %d, %d\n", mx, my,mz);
    float roll=0,pitch=0,yaw=0;
    MadgwickAHRSupdate((float)ax, (float)ay, (float)az, (float)gx, (float)gy, (float)gz, (float)mx, (float)my, (float)mz,&roll,&pitch,&yaw);
    
    //Formatting output
    snprintf(sensor_fusion_result,33,"Yaw: %4d Pitch: %4d Roll: %4d",(int)yaw,(int)pitch,(int)roll); 
    printf("Yaw: %f Pitch: %f Roll: %f\n",(yaw),(pitch),(roll));
    
    if( write(socket_client,sensor_fusion_result,33) < 0){
      perror("Couldnt write sensor results to socket\n");
    }
    
    free(sensor_fusion_result);
 }
}
  //shutdown(socket_client,SHUT_RDWR);
  
}
