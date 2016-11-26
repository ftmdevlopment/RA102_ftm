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
//#include "libtm.h"
extern char g_uart_recv_buf[100];
extern int g_uart_fd;

int speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300};
int name_arr[] = {115200, 38400, 19200, 9600, 4800, 2400, 1200, 300};

void set_speed(int fd, int speed)
{
	unsigned int   i; 
	int   status; 
	struct termios   Opt;

	tcgetattr(fd, &Opt); 
	for ( i= 0; i < sizeof(speed_arr) / sizeof(int); i++) 
	{ 
		if (speed == name_arr[i]) 
		{     
		    tcflush(fd, TCIOFLUSH);     
		    cfsetispeed(&Opt, speed_arr[i]); 
		    cfsetospeed(&Opt, speed_arr[i]);   

	    	status = tcsetattr(fd, TCSANOW, &Opt); 
			if (status != 0) 
			{    
		        printf("william: tcsetattr fd1"); 
		        return;     
			}    
	    	tcflush(fd,TCIOFLUSH);   
	    } 
	}
}


int set_parity(int fd,int databits,int stopbits,int parity)
{
    struct termios options;
	if (tcgetattr( fd,&options) != 0)
	{
	    printf("SetupSerial 1");
	    return -1;
	}

	options.c_cflag &= ~CSIZE;
	switch (databits)
	{
	    case 7:
	        options.c_cflag |= CS7;
	        break;
	    case 8:
	        options.c_cflag |= CS8;
	        break;
	    default:
	        fprintf(stderr,"Unsupported data size\n");
	        return -1;
    }
	switch (parity)
    {
	    case 'n':
	    case 'N':
	//        options.c_cflag &= ~PARENB;   /* Clear parity enable */
	//        options.c_iflag &= ~INPCK;     /* Enable parity checking */
	        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /*Input*/
	        options.c_oflag &= ~OPOST;   /*Output*/
	        break;
	    case 'o':
	    case 'O':
	        options.c_cflag |= (PARODD | PARENB); 
	        options.c_iflag |= INPCK;             /* Disnable parity checking */
	        break;
	    case 'e':
	    case 'E':
	        options.c_cflag |= PARENB;     /* Enable parity */
	        options.c_cflag &= ~PARODD;   /* 转换为偶效验*/ 
	        options.c_iflag |= INPCK;       /* Disnable parity checking */
	        break;

	    case 'S':
	    case 's': /*as no parity*/
	        options.c_cflag &= ~PARENB;
	        options.c_cflag &= ~CSTOPB;
	        break;
	    default:
	        fprintf(stderr,"Unsupported parity\n");
	        return  -1;
     }
 
	switch (stopbits)
	{
	    case 1:
	        options.c_cflag &= ~CSTOPB;
	        break;
	    case 2:
	        options.c_cflag |= CSTOPB;
	        break;
	    default:
	        fprintf(stderr,"Unsupported stop bits\n");
	        return  -1;
	}

	/* Set input parity option */
	if ((parity != 'n') && (parity != 'N'))
        options.c_iflag |= INPCK;

    options.c_cc[VTIME] = 5; // 0.5 seconds
    options.c_cc[VMIN] = 1;
    options.c_cflag &= ~HUPCL;
    options.c_iflag &= ~INPCK;
    options.c_iflag |= IGNBRK;
    options.c_iflag &= ~ICRNL;
    options.c_iflag &= ~IXON;
    options.c_lflag &= ~IEXTEN;
    options.c_lflag &= ~ECHOK;
    options.c_lflag &= ~ECHOCTL;
    options.c_lflag &= ~ECHOKE;
    options.c_oflag &= ~ONLCR;

	tcflush(fd, TCIFLUSH); /* Update the options and do it NOW */

	if (tcsetattr(fd,TCSANOW,&options) != 0)
    {
        printf("SetupSerial 3");
        return  -1;
    }

	return 0;
}

int uart_init_SAC(void)
{
	if (g_uart_fd == -1)
	{	
		g_uart_fd = open("/dev/ttySAC2", O_RDWR|O_NONBLOCK);
		if( g_uart_fd == -1)
	    	{
			printf("can not open /dev/ttySAC2!\n");
			return -1;
		}
	    	else
			printf("open /dev/ttySAC2 ok!\n");
		
		set_speed(g_uart_fd, 115200);
		if (set_parity(g_uart_fd, 8, 1, 'N')== -1)
		{
		    printf("Fail to set uart parity\n");
		    return -1;
		}
	}

	return 0;
}

int uart_uninit(void)
{
	if (g_uart_fd != -1)
   		close(g_uart_fd);

	g_uart_fd = -1;

	return 0;
}

int uart_send(char* str, int len)
{
	if(g_uart_fd== -1)
		return -1;

 	write(g_uart_fd, str, len);
	return 0;
}

int uart_recv(void)
{
	int ret = 0;
	char *cmd_start_flag = "AABBCCDD";
	char *cmd_end_flag = "AABBCCEF";

	if (g_uart_fd == -1)
		return -1;

	memset(g_uart_recv_buf, 0, sizeof(g_uart_recv_buf));
	ret = read(g_uart_fd, g_uart_recv_buf, sizeof(g_uart_recv_buf));
	printf("recv: %s\n", g_uart_recv_buf);
	if(strstr(g_uart_recv_buf, cmd_start_flag) && strstr(g_uart_recv_buf, cmd_end_flag))
		return 0;
	
	return -1;
}



// end of file
