
extern FILE* input;
extern FILE* output;
extern int testflag;

#define print(arg...) \
	if ( testflag == 1 && output != NULL) { \
		fprintf(output,##arg); \
		fflush(output); \
	}

#include <android/log.h>
#define ALOGD(...)__android_log_print (ANDROID_LOG_DEBUG, "engmode", __VA_ARGS__)

/*if engmode log print on tty,  LOG_PRINT_PLACE wil be 1,
 * else if save in logcat , LOG_PRINT_PLACE wil be 0*/
#define LOG_PRINT_PLACE  0
