#include <stdio.h>
#include <stdlib.h>
#include "ui.h"
#include "event.h"
#include "board_test.h"
#include "console.h"

extern int row , col, test_num;
void reboot_work(void)
{
	system("su");
	system("reboot");
}

static struct win main_win =
{
	.title = "engmode",
	.cur = 0,
	.n = 6,
	.menu[0] = {"Board Test", board_test_win_work},
	.menu[1] = {"Board Test by console", board_test_by_console},
	.menu[2] = {"RF Test", NULL},
	.menu[3] = {"Current Test", NULL},
	.menu[4] = {"Erase Userdata", NULL},
	.menu[5] = {"Reboot", reboot_work},
};

void main_win_work(struct win* win)
{
	struct event ev;
	draw_win(win);
	while(1)
	{
//		dequeue_event_locked(&ev);
		ev.type =  scan_key();
		if(ev.type == KEY_VOLUMEUP || ev.type == HOST_EV_UP)
		{
			fill_screen(back_color);
			col = 0;
			row = 0;
			test_num = 0;
			update_win(win, win->cur, win->cur-1);
		}
		else if(ev.type == KEY_VOLUMEDOWN || ev.type == HOST_EV_DOWN)
		{
			fill_screen(back_color);
			col = 0;
			row = 0;
			test_num = 0;
			update_win(win, win->cur, win->cur+1);
		}
		else if (ev.type == KEY_POWER || ev.type == HOST_EV_ENTER)
		{
			fill_screen(back_color);
			col = 0;
			row = 0;
			test_num = 0;
			if (win->menu[win->cur].action != NULL) {
				win->menu[win->cur].action();
				/* when menu action returns, we should redraw the windown */
				draw_win(win);
			}

		}
		wait_uart_order();
	}
}

static void welcome(void)
{
	print("****************\n");
	print("engmode start up\n");
	print("****************\n");
}

int main()
{
#if 1
	/*
	 * console should be the first,
	 * make sure do console_exit when system out
	 * */
//	console_init();
	virt_uart_init();
	uart_init_SAC();
	welcome();
	event_queue_init_need(1024);
	ui_init();
//	event_init();

	main_win_work(&main_win);

	event_queue_uninit();
	console_exit();
#endif
	return 0;
}
#if 0
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

/* for test only  */
int _main(int argc, char** argv)
{
	#if 0
	int fd = fopen("/dev/ttySAC2", O_WRONLY);

	if (argc < 2)
		return -1;

	write(argv[1], strlen(argv[1]), fd);

	return 0;
	#endif
	int i = 10;
	while(i --)
		system("echo engmode print > /dev/ttySAC3");

	return 0;
}
#endif
