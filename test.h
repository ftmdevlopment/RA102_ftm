#define OK 0
#define FAIL -1

struct test_list{
	char* name;
	int (*test)(void);
	char* cmd;
	char* result;
};
