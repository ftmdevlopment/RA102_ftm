#include <linux/input.h>
#include <sys/poll.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>

enum ev_type {
	NO_EV = 0,
	/* key map */
	KEY_EV_UP = KEY_VOLUMEUP,
	KEY_EV_DOWN = KEY_VOLUMEDOWN,
	KEY_EV_ENTER = KEY_POWER,
	HOST_EV_UP = 100000,
	HOST_EV_DOWN,
	HOST_EV_ENTER,
	HOST_EV_ASK,
	HOST_EV_SELECT,
	HOST_EV_BACK,
};

struct event {
	enum ev_type type;
	unsigned char* data; /* event data */
};

void event_init(void);
void process_event(char* buffer, int len);
/* int get_event(void); */

