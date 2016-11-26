#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <termios.h>
#include <stdlib.h>
#include <pthread.h>
#include "console.h"

#define DEBUG 0

FILE* output;
FILE* input;
//++ add detect thread
int testflag=0;
//-- add detect thread
static int g_virt_uart_fd = -1;
extern char g_uart_recv_buf;
static struct termios termios;

#define CONSOLE_TTY "/dev/ttyGS1"
//#define CONSOLE_TTY "/dev/ttySAC3"

static struct pollfd ufds;
//++ add detect thread
static struct pollfd ufds1[1];
//-- add detect thread
extern int g_virt_uart_fd;
static int uart_init()
{
	if (g_virt_uart_fd == -1)
	{
		g_virt_uart_fd = open(CONSOLE_TTY, O_RDWR);
		if (g_virt_uart_fd < 0)
		{
			printf("open %s failed\n",CONSOLE_TTY);
			return -1;
		}
		printf("open %s ok!\n",CONSOLE_TTY);
		
		tcflush(g_virt_uart_fd, TCIOFLUSH);
		tcgetattr(g_virt_uart_fd, &termios);

		tcflush(g_virt_uart_fd, TCIOFLUSH);
		cfsetospeed(&termios, B115200);
		cfsetispeed(&termios, B115200);
		tcsetattr(g_virt_uart_fd, TCSANOW, &termios);

	}
	return 0;
}


static void* uart_event_proc(void* args)
{
	char buffer[101];
	char* p;
	int n = -1;
	int len = 0;

	while (1)
	{
		if (n < 0) {
			p = buffer;
			buffer[99] = 0;
		} else
//++ add engmode log					
//			p = buffer+n;
			p = buffer+len;
//-- add engmode log		
		n = read(g_virt_uart_fd, p, 100 - len);
		len += n;
		if (p[n - 1] == '\n' || len == 100) {
			p[n] = 0;
//++ add engmode log
	//		if (DEBUG)
	//			print("%s", buffer);
			if (DEBUG){
				if (LOG_PRINT_PLACE){
					print("%s", buffer);
				}
				else{
					ALOGD("%s", buffer);
				}
			}
//-- add engmode log
			process_event(buffer, 100); /* process the uart data, enqueue a event or dismiss it */
			n = -1; /* reset the buffer */
			len = 0;
		}
	}

	/* should nerver reach here */
	return NULL;
}

//++ add a detect thread
void* console_detect_proc(void* args)
{
	output = NULL;
	input = NULL;
	int state;
	char statec[20];
	while(1)
	{
		state=open("/sys/class/android_usb/android0/state",O_RDONLY);
		read(state, statec, sizeof(statec));
		close(state);
		if ( statec[0]=='C' && output==NULL && input==NULL)
		{
			output = fopen(CONSOLE_TTY, "w");
			input = fopen(CONSOLE_TTY, "r");
			testflag = 1;
		}
		else if ( statec[0]=='D' && output !=NULL && input !=NULL)
		{
			fflush(output);
			fclose(output);
			output = NULL;
			fflush(input);
			fclose(input);
			input = NULL;
			testflag = 0;
		}
		sleep(1);
	}

 	return NULL;
}

void console_connect_detect(void)
{
	pthread_t p;
	p = pthread_create(&p, NULL, console_detect_proc, NULL);
	sleep(2);
}
//--add CONSOLE_TTY link judgement
void console_init(void)
{
	pthread_t p;
//++ add CONSOLE_TTY link judgement
/*
	uart_init();
	output = fopen(CONSOLE_TTY, "w");
	input = fopen(CONSOLE_TTY, "r");
*/
    int ret;
	ret = uart_init();
	if (ret != 0)
		return;

	console_connect_detect();
//-- add a detect thread
	ufds.fd = open(CONSOLE_TTY, O_RDWR|O_NONBLOCK);
	ufds.events = POLLIN;

	p = pthread_create(&p, NULL, uart_event_proc, NULL);
}
//++ add CONSOLE_TTY link judgement
/*
int get_input(char* buffer, int len)
{
	while (1) {
		poll(&ufds, 1, -1);
		if (ufds.revents & POLLIN) {
			return fread(buffer, 1, len, input);
		}
	}
}
*/
//-- add CONSOLE_TTY link judgement
void console_exit(void)
{
//++ add CONSOLE_TTY link judgement
/*
	fflush(output);
	fflush(input);
	fclose(output);
	fclose(input);
*/
	if (output != NULL)
	{
		fflush(output);
		fclose(output);
	}
	if (input != NULL)
	{
		fflush(input);
		fclose(input);
	}
	g_virt_uart_fd = -1;
//++ add CONSOLE_TTY link judgement
}

#if 0
int virt_uart_send(char* str, int len)
{
	if(g_virt_uart_fd== -1)
		return -1;

 	write(g_virt_uart_fd, str, len);
	return 0;
}

int virt_uart_recv(void)
{
	int ret = 0;
	char *cmd_start_flag = "AABBCCDD";
	char *cmd_end_flag = "AABBCCEF";
	if (g_virt_uart_fd == -1)
		return -1;

	memset(g_uart_recv_buf, 0, sizeof(g_uart_recv_buf));
	ret = read(g_virt_uart_fd, g_uart_recv_buf, sizeof(g_uart_recv_buf));
	printf("virtual recv: %s\n", g_uart_recv_buf);
	if(strstr(g_uart_recv_buf, cmd_start_flag) && strstr(g_uart_recv_buf, cmd_end_flag))
		return 0;
	
	return -1;
}
#endif
