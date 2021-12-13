#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFFER_MAX 3
#define DIRECTION_MAX 45

#define IN 0
#define OUT 1
#define PWM 0

#define LOW 0
#define HIGH 1
#define VALUE_MAX 256

#define ON 4 //state of the fan
#define OFF 5

#define BAD 1 //state of the air
#define GOOD 0

int delay = 3; //delay is int number and plused linearly. 

int insid_state = GOOD;
int outsid_state = GOOD;

char* addrnum1 = NULL;
char* addrnum2 = NULL;
char* portnum1 = NULL;
char* portnum2 = NULL;
char* portnum3 = NULL;

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}


static int PWMExport(int pwmnum)
{
	char buffer[BUFFER_MAX];
	int bytes_written;
	int fd;
	
	fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
	if ( -1 == fd) {
		fprintf(stderr, "Failed to open in export!\n");
		return(-1);
		}
	
	bytes_written = snprintf(buffer,BUFFER_MAX, "%d", pwmnum);
	write(fd, buffer, bytes_written);
	close(fd);
	
	sleep(1);
	return(0);
}

static int PWMUnexport(int pwmnum)
{
	char buffer[BUFFER_MAX];
	int bytes_written;
	int fd;
	
	fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
	if ( -1 == fd) {
		fprintf(stderr, "Failed to open in unexport!\n");
		return(-1);
		}
	
	bytes_written = snprintf(buffer,BUFFER_MAX, "%d", pwmnum);
	write(fd, buffer, bytes_written);
	close(fd);
	
	sleep(1);
	return(0);
}

static int PWMEnable(int pwmnum)
{
	static const char s_unenable_str[] = "0";
	static const char s_enable_str[] = "1";
	
	char path[DIRECTION_MAX];
	int fd;
		
	snprintf(path, DIRECTION_MAX,"/sys/class/pwm/pwmchip0/pwm%d/enable",pwmnum);
	fd = open(path, O_WRONLY);
	if (-1 == fd){
		fprintf(stderr, "Failed to open in enable!\n");
		return -1;
	}
		
	write(fd, s_unenable_str, strlen(s_unenable_str));
	close(fd);
		
	fd = open(path, O_WRONLY);
	if(-1 == fd) {
		fprintf(stderr, "Failed to open in enable!\n");
		return -1;
	}
		
	write(fd, s_enable_str, strlen(s_enable_str));
	close(fd);
	return(0);
}
static int PWMUnable(int pwmnum)
{
	static const char s_unable_str[] = "0";
	char path[DIRECTION_MAX];
	int fd;
		
	snprintf(path, DIRECTION_MAX,"/sys/class/pwm/pwmchip0/pwm%d/enable",pwmnum);
	fd = open(path, O_WRONLY);
	if (-1 == fd){
		fprintf(stderr, "Failed to open in enable!\n");
		return -1;
	}
		
	write(fd, s_unable_str, strlen(s_unable_str));
	close(fd);

	return(0);
}

static int PWMWritePeriod(int pwmnum, int value)
{
	char s_values_str[VALUE_MAX];
	char path[VALUE_MAX];
	int fd, byte;
	
	snprintf(path, VALUE_MAX,"/sys/class/pwm/pwmchip0/pwm%d/period",pwmnum );
	fd = open(path, O_WRONLY);
	if(-1 == fd) {
		fprintf(stderr, "Failed to open in period!\n");
		return(-1);
	}
	
	byte = snprintf(s_values_str, 10, "%d", value);
	if(-1 == write(fd, s_values_str, byte)) {
		fprintf(stderr, "Failed to write value in period!\n");
		close(fd);
		return(-1);
	}
	
	close(fd);
	return(0);
}

static int PWMWriteDutyCycle(int pwmnum, int value)
{
	char path[VALUE_MAX];
	char s_values_str[VALUE_MAX];
	int fd, byte;
	
	snprintf(path, VALUE_MAX,"/sys/class/pwm/pwmchip0/pwm%d/duty_cycle",pwmnum );
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in duty_cycle!\n");
		return(-1);
	}
	
	byte = snprintf(s_values_str, 10, "%d", value);
	
	if(-1 == write(fd, s_values_str, byte)) {
		fprintf(stderr, "Failed to write value in duty_cycle\n");
		close(fd);
		return(-1);
	}
	
	close(fd);
	return(0);
}

void *fan_thd(){

  int fan_state = OFF;
   
	PWMExport(PWM);
	PWMWritePeriod(PWM, 20000000);
	PWMWriteDutyCycle(PWM, 500000);
	PWMEnable(PWM);
	
	while(1){
 
     if(insid_state == GOOD)
       fan_state = OFF;
     else if(insid_state ==  BAD && outsid_state == GOOD)
       fan_state = ON;
     else if(insid_state == BAD && outsid_state == BAD)
       fan_state = OFF;
     
     if(fan_state == ON) //when fan_state is ON, motor moves.
     {  
        for(int i = 500000; i < 2500000; i += 100000){
           PWMWriteDutyCycle(PWM, i);
           usleep(-delay * 5000 + 40000);
         }
       
        for(int i = 2500000; i > 500000; i -= 100000){
          PWMWriteDutyCycle(PWM, i);
          usleep(-delay * 5000 + 40000);
        }
     }		
	}
 
	exit(0);
}	

void *socket_sensor_state_thd(){

  int sock;
  struct sockaddr_in serv_addr;
  char msg[2];
  char on[2]="1";
  int str_len;
  
  //socket initialize
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
      error_handling("socket() error");
        
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(addrnum1);
    serv_addr.sin_port = htons(atoi(portnum1));  
        
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
            error_handling("connect() error");
      
    //message communication
    while(1){   
               
        str_len = read(sock, msg, sizeof(msg));
        if(str_len == -1)
            error_handling("read() error");
            
        printf("Receive message from Seonsor Server(state) : %s\n",msg);
        if(strncmp(on,msg,1)==0)
            insid_state = BAD;
        else
            insid_state = GOOD;
        
    }
    close(sock); 

    return(0);


}

void *socket_sensor_delay_thd(){

  int sock;
  struct sockaddr_in serv_addr;
  char msg[1024];
  int str_len;
  
  //socket initialize
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
      error_handling("socket() error");
        
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(addrnum1);
    serv_addr.sin_port = htons(atoi(portnum2));  
        
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
            error_handling("connect() error");
      
    //message communication
    while(1){   
               
        str_len = read(sock, msg, sizeof(msg));
        if(str_len == -1)
            error_handling("read() error");
            
        printf("Receive message from Seonsor Server(delay) : %s\n",msg);
        
        delay = atoi(msg);//delay updated
        delay = delay / 10;
        
    }
    close(sock); 

    return(0);


}


void *socket_api_thd(){

  int sock;
  struct sockaddr_in serv_addr;
  char msg[2];
  char on[2]="1";
  int str_len;
  
  //socket initialize
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
      error_handling("socket() error");
        
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(addrnum2);
    serv_addr.sin_port = htons(atoi(portnum3));  
        
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
            error_handling("connect() error");
      
    //message communication
    while(1){   
               
        str_len = read(sock, msg, sizeof(msg));
        if(str_len == -1)
            error_handling("read() error");
            
        printf("Receive message from API Server : %s\n",msg);
        if(strncmp(on,msg,1)==0)
            outsid_state = BAD;
        else
            outsid_state = GOOD;
        
    }
    close(sock); 

    return(0);

}

int main(int argc, char *argv[]){
	
	pthread_t pthread1, pthread2, pthread3, pthread4;
 
  if(argc!=6){
        printf("Usage : <seonsor server address1> <port1> <port2> <api server address2> <port3>\n");
    }
  
  //global variable initialize
  addrnum1 = argv[1];
  portnum1 = argv[2];
  portnum2 = argv[3];
  
  addrnum2 = argv[4];
  portnum3 = argv[5];
 
 //4 pthread create
	pthread_create(&pthread1, NULL,fan_thd, NULL);
	usleep(100000);
	pthread_create(&pthread2, NULL, socket_sensor_state_thd, NULL);
  usleep(100000);
	pthread_create(&pthread3, NULL, socket_sensor_delay_thd, NULL);
	usleep(100000);
	pthread_create(&pthread4, NULL, socket_api_thd, NULL);
 
  //pthread join
	usleep(600000000);
	pthread_join(pthread1, NULL);
	pthread_join(pthread2, NULL);
  pthread_join(pthread3, NULL);
  pthread_join(pthread4, NULL);
  
  //prepare the end
	
	PWMUnexport(PWM);
	
	return 0;
}
