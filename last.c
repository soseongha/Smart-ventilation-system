#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <wiringPi.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#define _CRT_SECURE_NO_WARNINGS
#define MAXTIMINGS    85
#define DHTPIN        7 //BCM 24
 
#define BUFFER_MAX 3
#define DIRECTION_MAX 45

#define IN  0
#define OUT 1
#define PWM 0 

#define LOW  0
#define HIGH 1
#define VALUE_MAX    256

#define POUT 21
#define PIN  20

char* portnum1 = NULL;
char* portnum2 = NULL;

int gas = 0;
double distance = 0;
#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])
static const char* DEVICE = "/dev/spidev0.0";
static uint8_t MODE = SPI_MODE_0;
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;

int dht11_dat[5] = { 0, 0, 0, 0, 0 };
int fire =0;
int water =0;
int gijun= 800;

static int
PWMExport(int pwmnum)
{
	char buffer[BUFFER_MAX];
	int bytes_written;
	int fd;

	fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in export!\n");
		return(-1);
	}
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
	write(fd, buffer, bytes_written);
	close(fd);
	sleep(1);
	return(0);
}

static int
PWMUnexport(int pwmnum) {
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in unexport!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
	write(fd, buffer, bytes_written);
	close(fd);

	sleep(1);

	return(0);
}

static int
PWMEnable(int pwmnum)
{
	static const char s_unenable_str[] = "0";
	static const char s_enable_str[] = "1";

	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in enable!\n");
		return -1;
	}

	write(fd, s_unenable_str, strlen(s_unenable_str));
	close(fd);

	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in enable!\n");
		return -1;
	}

	write(fd, s_enable_str, strlen(s_enable_str));
	close(fd);
	return(0);
}

static int
PWMUnable(int pwmnum)
{
	static const char s_unable_str[] = "0";
	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in enable!\n");
		return -1;
	}

	write(fd, s_unable_str, strlen(s_unable_str));
	close(fd);

	return(0);
}

static int
PWMWritePeriod(int pwmnum, int value)
{
	char s_values_str[VALUE_MAX];
	char path[VALUE_MAX];
	int fd, byte;

	snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/period", pwmnum);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in period!\n");
		return(-1);
	}

	byte = snprintf(s_values_str, 10, "%d", value);

	if (-1 == write(fd, s_values_str, byte)) {
		fprintf(stderr, "Failed to write value in period!\n");
		close(fd);
		return(-1);
	}

	close(fd);
	return(0);
}

static int
PWMWriteDutyCycle(int pwmnum, int value)
{
	char path[VALUE_MAX];
	char s_values_str[VALUE_MAX];
	int fd, byte;

	snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open in duty_cycle!\n");
		return(-1);
	}


	byte = snprintf(s_values_str, 10, "%d", value);

	if (-1 == write(fd, s_values_str, byte)) {
		fprintf(stderr, "Failed to write value! in duty_cycle\n");
		close(fd);
		return(-1);
	}

	close(fd);
	return(0);
}

static int GPIOExport(int pin) {

	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

static int GPIOUnexport(int pin) {
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return(-1);
	}	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

static int GPIODirection(int pin, int dir) {
	static const char s_directions_str[] = "in\0out";

	char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio direction for writing!\n");
		return(-1);
	}

	if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

static int GPIORead(int pin) {
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for reading!\n");
		return(-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		return(-1);
	}

	close(fd);

	return(atoi(value_str));
}

static int GPIOWrite(int pin, int value) {
	static const char s_values_str[] = "01";
	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio value for writing!\n");
		return(-1);
	}

	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return(-1);

		close(fd);
		return(0);
	}
}

void error_handling(char *message){
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}

void *state_thd(){


    int serv_sock,clnt_sock=-1;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;
    char msg[2] = "1";

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1)
        error_handling("socket() error");
        
    memset(&serv_addr, 0 , sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(portnum1));
    
    if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
        error_handling("bind() error");
    if(listen(serv_sock,5) == -1)
            error_handling("listen() error");

    if(clnt_sock<0){           
            clnt_addr_size = sizeof(clnt_addr);
            clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, 	&clnt_addr_size);
            if(clnt_sock == -1)
                error_handling("accept() error");   
        }
   
   
     while(1)
    {           
		if(gas>=500)
			strcpy(msg,"1");
		else
			strcpy(msg,"0");
			
        write(clnt_sock, msg, sizeof(msg));
        printf("msg = %s\n",msg);
        usleep(500 * 10000);
    }

    close(clnt_sock);
    close(serv_sock);

    exit(0);

}
void *delay_thd(){



    int serv_sock,clnt_sock=-1;
    struct sockaddr_in serv_addr,clnt_addr;
    socklen_t clnt_addr_size;
    char msg[1024] = "25";

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1)
        error_handling("socket() error");
        
    memset(&serv_addr, 0 , sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(portnum2));
    
    if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
        error_handling("bind() error");
    if(listen(serv_sock,5) == -1)
            error_handling("listen() error");

    if(clnt_sock<0){           
            clnt_addr_size = sizeof(clnt_addr);
            clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, 	&clnt_addr_size);
            if(clnt_sock == -1)
                error_handling("accept() error");   
        }
   
	int sum =0;
     while(1)
    {           
		sum = water + fire;
		sprintf(msg, "%d", sum);
        write(clnt_sock, msg, sizeof(msg));
        printf("msg = %s\n",msg);
        usleep(500 * 10000);
    }

    close(clnt_sock);
    close(serv_sock);

    exit(0);

}
static int prepare(int fd) {
	if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1) {
		perror("Can't set MODE");
		return -1;
	}

	if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1) {
		perror("Can't set number of BITS");
		return -1;
	}

	if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1) {
		perror("Can't set write CLOCK");
		return -1;
	}
	if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1) {
		perror("Can't set read CLOCK");
		return -1;
	}
	return 0;
}
uint8_t control_bits_differential(uint8_t channel) {
	return (channel & 7) << 4;
}
uint8_t control_bits(uint8_t channel) {
	return 0x8 | control_bits_differential(channel);
}
int readadc(int fd, uint8_t channel) {
	uint8_t tx[] = { 1, control_bits(channel), 0 };
	uint8_t rx[3];
	struct spi_ioc_transfer tr = {
	   .tx_buf = (unsigned long)tx,
	   .rx_buf = (unsigned long)rx,
	   .len = ARRAY_SIZE(tx),
	   .delay_usecs = DELAY,
	   .speed_hz = CLOCK,
	   .bits_per_word = BITS,
	};
	if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) {
		perror("IO Error");
		abort();
	}
	return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}
void read_dht11_dat()
{
    uint8_t laststate    = HIGH;
    uint8_t counter        = 0;
    uint8_t j        = 0, i;
    float    f; 
 
    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;
 
    pinMode( DHTPIN, OUTPUT );
    digitalWrite( DHTPIN, LOW );
    delay( 18 );
    digitalWrite( DHTPIN, HIGH );
    delayMicroseconds( 40 );
    pinMode( DHTPIN, INPUT );
 
    for ( i = 0; i < MAXTIMINGS; i++ )
    {
        counter = 0;
        while ( digitalRead( DHTPIN ) == laststate )
        {
            counter++;
            delayMicroseconds( 1 );
            if ( counter == 255 )
            {
                break;
            }
        }
        laststate = digitalRead( DHTPIN );
 
        if ( counter == 255 )
            break;
 
        if ( (i >= 4) && (i % 2 == 0) )
        {
            dht11_dat[j / 8] <<= 1;
            if ( counter > 50 /*16*/ )
                dht11_dat[j / 8] |= 1;
            j++;
        }
    }
 
    if ( (j >= 40) &&
         (dht11_dat[4] == ( (dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF) ) )
    {
        f = dht11_dat[2] * 9. / 5. + 32;
        printf( "Humidity = %d.%d %% Temperature = %d.%d C (%.1f F)\n",dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f ); 
        water = dht11_dat[0];
        fire =dht11_dat[2];
    }else  {
        //printf( "Data not good, skip\n" );
    }
}
void *fire_thd() {
	if ( wiringPiSetup() == -1 )
        exit( 1 );
 
    while ( 1 )
    {
        read_dht11_dat();
        delay( 2000 ); 
    }
 
    exit(0);
}
void *led_thd() {
	int target_bright = 0;
	int prev_bright = 0;

	PWMExport(PWM); // pwm0 is gpio18 
	PWMWritePeriod(PWM, 20000000);
	PWMWriteDutyCycle(PWM, 0);
	PWMEnable(PWM);

	while (1) {
		target_bright = (int)1 / (gas+1) * 22000 * 900;

		if (target_bright > prev_bright) {
			for (int i = prev_bright; i < target_bright; i += 4000) {
				//printf("ligth\n");
				PWMWriteDutyCycle(PWM, i);
				usleep(1000);
			}
		}

		else if (target_bright <= prev_bright) {
			for (int i = prev_bright; i > target_bright; i -= 4000) {
				PWMWriteDutyCycle(PWM, i);
				//printf("ligth\n");
				usleep(1000);
			}
		}
		prev_bright = target_bright;
		usleep(100000);
	}
	exit(0);
}
void* readgas_thd()
{
	int fd = open(DEVICE, O_RDWR);

	if (prepare(fd) == -1) {
		exit(0);
	}
	while (1) {
		gas = readadc(fd, 0);
		printf("gas = %d\n", gas);
		usleep(1000000);
	}
	//close(fd);
}
int main(int argc, char** argv) {
	
	pthread_t gas_thread;
	pthread_t led_thread;
	pthread_t fire_thread;
	
	
	pthread_t pthread1, pthread2;
 
    if(argc!=3){
        printf("Usage : %s <state_port1> <delay_port2>\n",argv[0]);
    }
    
  //global variable initialize
	portnum1 = argv[1];
	portnum2 = argv[2];
  
  //4 pthread create
	pthread_create(&gas_thread, NULL, readgas_thd, NULL);
	pthread_create(&led_thread, NULL, led_thd, NULL);
	pthread_create(&fire_thread, NULL, fire_thd, NULL);
	pthread_create(&pthread1, NULL, state_thd, NULL);
	usleep(100000);
	pthread_create(&pthread2, NULL, delay_thd, NULL);
 
  //pthread join
	usleep(600000000);
	pthread_join(pthread1, NULL);
	pthread_join(pthread2, NULL);
    pthread_cancel(gas_thread);
	pthread_cancel(led_thread);
	pthread_cancel(fire_thread);
	
    return(0);
}
