#include <termios.h>
#include <fcntl.h>

#define UART "/dev/ttyS1"
#define CONSOLE "/dev/tty"

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
		write(print_fd, buffer, 100);
	}
}

#include <string.h>

static void echo()
{
	char* str = "hello world\n";
	write(uart_fd, str, strlen(str));
}

int main()
{
	int err;
	err = uart_init();
	if (err != 0)
		return -1;
	//print();
	echo();
	return 0;
}

