void board_test_win_work(void);
void Erase_cache_data_partition_win(void);
struct TESTARRAY{
	char* name;
	int (*handler)(void);
	char* cmd;
	char* result;
};

#define PROPERTY_VALUE_MAX 50
#define UART_PORT_FIQ 0
#define UART_PORT_GS0 1
extern void board_test_by_console(void);
