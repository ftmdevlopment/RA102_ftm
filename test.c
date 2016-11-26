#include <termios.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <stdlib.h>

#define UART "/dev/ttySAC2"
#define CONSOLE "/dev/tty"

#include <unistd.h>

static int uart_fd = -1;
static struct termios termios;

static int uart_init()
{
	uart_fd = open(UART, O_RDWR);
	if (uart_fd < 0)
		return -1;

	tcflush(uart_fd, TCIOFLUSH);
	tcgetattr(uart_fd, &termios);

	tcflush(uart_fd, TCIOFLUSH);
	cfsetospeed(&termios, B115200);
	cfsetispeed(&termios, B115200);
	tcsetattr(uart_fd, TCSANOW, &termios);

	return 0;
}

static void print()
{
	char buffer[100];
	int n;
	int print_fd = open(CONSOLE, O_WRONLY);

	if (print_fd < 0)
		return;

	while (n = read(uart_fd, buffer, 100))
	{
		write(print_fd, buffer, n);
	}
}

struct pollfd pollfd;

static void print_poll()
{
	char buffer[100];
	int n;
	int print_fd = open(CONSOLE, O_WRONLY);

	if (print_fd < 0)
		return;
	pollfd.fd = uart_fd;
	pollfd.events = POLLIN|POLLRDNORM;

	while(1)
	{
		poll(&pollfd, 1, -1);
		n = read(pollfd.fd, buffer, 100);
		write(print_fd, buffer, n);
	}

}

int main()
{
	int err;
	err = uart_init();
	if (err != 0)
		return -1;
	print_poll();
	return 0;
}

