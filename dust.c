#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define IN  0
#define OUT 1

#define LOW  0
#define HIGH 1

#define POUT 18
#define VALUE_MAX 40

char* portnum1 = NULL;
char value[2];

static int
GPIOExport(int pin)
{
#define BUFFER_MAX 3
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

static int
GPIOUnexport(int pin)
{
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

static int
GPIODirection(int pin, int dir)
{
	static const char s_directions_str[] = "in\0out";

#define DIRECTION_MAX 35
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
		close(fd);
		return(-1);
	}

	close(fd);
	return(0);
}

static int
GPIORead(int pin)
{
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
		close(fd);
		return(-1);
	}

	close(fd);

	return(atoi(value_str));
}

static int
GPIOWrite(int pin, int value)
{
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
		close(fd);
		return(-1);
	}

	close(fd);
	return(0);
}

void error_handling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

void* state_thd() {


	int serv_sock, clnt_sock = -1;
	struct sockaddr_in serv_addr, clnt_addr;
	socklen_t clnt_addr_size;

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (serv_sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(portnum1));

	if (bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1)
		error_handling("listen() error");

	if (clnt_sock < 0) {
		clnt_addr_size = sizeof(clnt_addr);
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
		if (clnt_sock == -1)
			error_handling("accept() error");
	}


	while (1)
	{
		write(clnt_sock, value, sizeof(value));
		printf("value = %s\n", value);
		usleep(500 * 10000);
	}

	close(clnt_sock);
	close(serv_sock);

	exit(0);

}

void* api_thd()
{
	FILE* fp;
	char str[100];
	char ans[100] = "pm10Value";
	int num = 0;
	int save;

	if (-1 == GPIOExport(POUT))
		exit(1);

	if (-1 == GPIODirection(POUT, OUT))
		exit(2);

	while (1) {
		fp = fopen("input.txt", "r");      //Output.txt 파일을 엽니다.

		if (fp == NULL)                     //예외처리
		{
			printf("Read Error");
			exit - 1;
		}

		while (!feof(fp))                   //txt파일에 pm10Value를 찾아 그 단어가 포함된 줄을 str에 저장합니다.
		{
			num++;
			fgets(str, 100, fp);

			if (strstr(str, ans) != NULL)
			{
				break;
			}
		}

		fclose(fp);                         //txt파일을 닫습니다.

		char* tk1 = strtok(str, "<>");      // strtok를 이용해 저희가 원하는 미세먼지값만 tk3에 저장합니다.
		char* tk2 = strtok(NULL, "<>");
		char* tk3 = strtok(NULL, "<>");

		save = atoi(tk3);                   //문자형인 tk3를 정수형으로 변환합니다.

		if (0 <= save && save < 80)         //미세먼지값이 0~80이면 value에 0을 저장하고 Good을 출력
		{
			value[0] = '0';
			value[1] = '\0';
			printf("Dust is Good\n");
		}
		else if (save >= 80)                //미세먼지값이 80이상이면 value에 1을 저장하고 Bad를 출력
		{
			value[0] = '1';
			value[1] = '\0';
			printf("Dust is Bad\n");
		}

		if (value[0] == '1')                //value값이 1이면 led불이 켜짐
			GPIOWrite(POUT, HIGH);
		else if (value[0] == '0')           //value값이 0이면 led불이 꺼짐
			GPIOWrite(POUT, LOW);
		usleep(1000000);
	}
	if (-1 == GPIOUnexport(POUT))
		exit(4);
	exit(0);
}


int main(int argc, char* argv[]) {

	pthread_t pthread1, pthread2;

	if (argc != 2) {
		printf("Usage : %s <state_port1>\n", argv[0]);
	}

	//global variable initialize
	portnum1 = argv[1];

	//4 pthread create
	pthread_create(&pthread1, NULL, state_thd, NULL);
	usleep(100000);
	pthread_create(&pthread2, NULL, api_thd, NULL);

	//pthread join
	usleep(600000000);
	pthread_join(pthread1, NULL);
	pthread_join(pthread2, NULL);

	return(0);


}