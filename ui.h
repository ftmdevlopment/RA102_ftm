#define BLUE_COLOR		0x000000FF
#define GREEN_COLOR		0x0000FF00
#define RED_COLOR		0x00FF0000
#define WHITE_COLOR		0x00FFFFFF
#define BLACK_COLOR		0x00000000

/* TODO: get lcd size by a better way */
#if 0
#define LCD_WIDTH        720
#define LCD_HEIGHT       720 /* 1280 */
#endif
#define PIXEL_TYPE unsigned int
#define FONTHEIGHT	42
#define FONTWIDTH	24
#define ROWVALUE	(LCD_HEIGHT/FONTHEIGHT)
#define COLVALUE    (LCD_WIDTH/FONTWIDTH)

#define font_color BLACK_COLOR
#define back_color WHITE_COLOR
#define sep_color 0xD3D7CF
#define select_color 0xFCE94F
#define na_color BLUE_COLOR
#define ok_color GREEN_COLOR
#define fail_color RED_COLOR
extern int max_row;
extern int max_col;


extern unsigned char ac16x32CN[];

struct item
{
	char* name;
	void (* action) (void);
};

struct win
{
	char* title;
	int cur;
	int n;
	void (*win_work)(struct win*);
	struct item menu[100];
};



int ui_init(void);
void fill_screen(unsigned int color);
void ui_putc(char c, int row, int col, int color);
void ui_puts(const char* str, int row, int col, int color);
void ui_puts_select(const char* str, int row, int col);
int draw_back(int row, int color);
int draw_title(struct win* win);
void draw_win(struct win* win);
void update_win(struct win*, int, int);

