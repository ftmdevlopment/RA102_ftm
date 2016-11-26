//
// serial.c
//

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
extern char g_uart_recv_buf[100];
extern int g_virt_uart_fd;

int virt_uart_init(void)
{
	if (g_virt_uart_fd == -1)
	{
		g_virt_uart_fd = open("/dev/ttyGS1", O_RDWR|O_NONBLOCK);
		if( g_virt_uart_fd == -1)
		{
			printf("can not open /dev/ttyGS1!\n");
			return -1;
		}
        	else
			printf("open /dev/ttyGS1 ok\n");
		
		set_speed(g_virt_uart_fd, 115200);
		if (set_parity(g_virt_uart_fd, 8, 1, 'N')== -1)
		{
			printf("Fail to set uart parity\n");
			return -1;
		}
	}

	return 0;
}

int virt_uart_uninit(void)
{
	if (g_virt_uart_fd != -1)
   		close(g_virt_uart_fd);

	g_virt_uart_fd = -1;

	return 0;
}

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



// end of file
