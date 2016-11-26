#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

#include "ui.h"
#include "console.h"

static int fb = -1;
static int g_fb_size = -1;
static struct fb_fix_screeninfo g_fi;
static struct fb_var_screeninfo g_vi;
static void *bits = NULL;
static unsigned int* buff;
int max_row = 0;
int max_col = 0;
static int LCD_WIDTH = 0;
static int LCD_HEIGHT = 0;

int ui_init(void)
{
	fb = open("/dev/graphics/fb0", O_RDWR);
	if(fb < 0)
		return -1;

	if (ioctl(fb, FBIOGET_FSCREENINFO, &g_fi) < 0) {
		return -1;
	}

	if (ioctl(fb, FBIOGET_VSCREENINFO, &g_vi) < 0) {
		return -1;
	}

	LCD_WIDTH = g_vi.xres;
	LCD_HEIGHT = g_vi.yres;
	g_fb_size = g_vi.xres * g_vi.yres * 4;
	bits = mmap(0, g_fb_size * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
	buff = (unsigned int*) bits;
	if (bits == MAP_FAILED){
		return -1;
	}

	max_row = ROWVALUE;
	max_col = COLVALUE;

	return 0;
}

void fill_screen(unsigned int color)
{
    unsigned int i, j;
    unsigned int *pbuf = (unsigned int*)bits;

	if (pbuf == NULL)
		return;
	for (i = 0; i < LCD_HEIGHT; i++){
		for (j = 0; j < LCD_WIDTH; j++)
			pbuf[i * LCD_WIDTH + j] = color;
	}
	return;
}


void fill_screen_rgb(unsigned int* buffer)
{
	unsigned int i, j, k;
    unsigned int *pbuf = (unsigned int*)bits;

	if (pbuf == NULL)
		return;
	k = 0;
	for (i = 0; i < LCD_HEIGHT; i++){
		for (j = 0; j < LCD_WIDTH; j++)
			pbuf[i * LCD_WIDTH + j] = buffer[k++];
	}
	return;

}


//int back_color = WHITE_COLOR;
//int font_color = BLACK_COLOR;
//int sep_color = 0xD3D7CF;
//int select_color = 0xFCE94F;


void ui_putc(char c, int row, int col, int color)
{
	unsigned char * font, f;
	int i, j, x, y, index, r;

	if (c < 32 || c >= 127)
		return;
	if (row >= max_row ||
			row < 0 ||
			col >= max_col ||
			col < 0)
		return;

	font = &ac16x32CN[(c-32) * 126];

	for (j = 0; j < FONTHEIGHT; j++) {
		/* the index of a font in row */
		index = (row*FONTHEIGHT+j) * LCD_WIDTH + (col*FONTWIDTH);
		/* echo bit of *font stands for a pix, echo byte has 8 bit, left shift 3 bit */
		for (r = 0; r < 3; r++){
			f = *font;
			for (i = 7; i != -1; i--) {
				if ((f >> i) & 0x1)
					buff[index] = color;
				index++;
			}
			font++;
		}
	}

	return;
}

void ui_puts(const char* str, int row, int col, int color)
{
	int len = strlen(str);
	int i;

	for (i = 0; i < len; i++)
		ui_putc(str[i], row, i + col, color);
}

void ui_puts_select(const char* str, int row, int col)
{
	draw_back(row, select_color);
	ui_puts(str, row, col, font_color);
}


void ui_puts_mid(const char* str, int row, int color)
{
	int col = max_col - strlen(str);
	col = col >> 1; /* middle place */

	ui_puts(str, row, col, color);
}

void ui_puts_right(const char* str, int row, int color)
{
	int col = max_col - strlen(str) - 1;

	ui_puts(str, row, col, color);
}

int draw_back(int row, int color)
{
	int index, len, i;

	if (row > max_row)
		return -1;

	len = FONTHEIGHT * LCD_WIDTH;
	index = row * len;

	for (i = 0; i < len; i++)
		buff[index + i] = color;

	return 1;
}

int draw_title(struct win *win)
{
	int i;
	ui_puts_mid(win->title, 0, font_color);
	draw_back(1, sep_color);

	return 2;
}

#define  nkey  4
static key_color[nkey] = {0x0, 0x0 };

static void draw_vkey(void)
{
	int i, j, k, len, x, y, index;

	len = FONTHEIGHT * LCD_WIDTH;

	for (i = 0; i < nkey; i++) {
		index = len * (max_col - 1) + LCD_WIDTH / nkey * i;
		for (k = 0; k < FONTHEIGHT; k++) {
			index += k * LCD_WIDTH;
			for (j = 0; j < LCD_WIDTH / nkey; j++) {
				buff[index++] = key_color[i];
			}
		}
	}
}

void text_clear(void)
{
	print("\033[2J");
}
void draw_win(struct win * win)
{
	int row = 0;
	int i;

	fill_screen(back_color);
	row += draw_title(win);

	if (win->n == 0)
		return;

	text_clear();
	for (i = 0; i < win->n; i++)
		if (i != win->cur) {
			ui_puts(win->menu[i].name, row++, 1, font_color);
			print(" %s\n", win->menu[i].name);
		} else {
			ui_puts_select(win->menu[i].name, row++, 1);
			print(">%s\n", win->menu[i].name);
		}

	print("enter cmd: ");

	/* draw_vkey(); */
	return;
}

void update_win(struct win* win, int old, int new)
{
	if (new >= 0 && new < win->n && win->cur == old) {
#if 0
		draw_back(2+old, back_color);
		ui_puts(win->menu[old].name, 2+old, 1, font_color);
		ui_puts_select(win->menu[new].name, 2+new, 1);
		win->cur = new;
#endif
		win->cur = new;
	}
	draw_win(win);
}


