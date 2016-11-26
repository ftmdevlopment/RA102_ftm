#include "event.h"
#include "config.h"
#include "ui.h"
#include <sys/poll.h>
#include "event_queue.h"
#include <pthread.h>
#include "console.h"
#include <string.h>

#define DEBUG 0

static struct pollfd *ufds;
static int nfds;

static int open_device(const char *device)
{
	int fd;
	struct pollfd *new_ufds;

	fd = open(device, O_RDWR|O_NONBLOCK);
	new_ufds = realloc(ufds, sizeof(ufds[0]) * (nfds + 1));
	if(new_ufds == NULL)
		return -1;
	ufds = new_ufds;

	ufds[nfds].fd = fd;
	ufds[nfds].events = POLLIN;
	nfds++;

	return 0;
}

static int scan_dir(const char *dirname)
{
	char devname[PATH_MAX];
	char *filename;
	DIR *dir;
	struct dirent *de;
	dir = opendir(dirname);
	if(dir == NULL)
		return -1;
	strcpy(devname, dirname);
	filename = devname + strlen(devname);
	*filename++ = '/';
	while((de = readdir(dir))) {
		if(de->d_name[0] == '.' &&
				(de->d_name[1] == '\0' ||
				 (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;
		strcpy(filename, de->d_name);
		open_device(devname);
	}
	closedir(dir);
	return 0;
}

static int get_event(void);

void* key_event_proc(void* args)
{
	int ev;
	struct event event;
	event.data = NULL;
//++ add engmode log
	while(1) {
		event.type = get_event();
		if (DEBUG){
			if (LOG_PRINT_PLACE){
				print("enqueue event\n");
			}
			else{
				ALOGD("enqueue event\n");
			}
		}
		enqueue_event_locked(event);
	}
//-- add engmode log
	return NULL;
}


extern int proc_event(struct input_event *event);
int scan_key(void)
{

	nfds = 0;
	ufds = NULL;
	int ev;
	struct event event;
	int i;
	struct input_event input;
	int ret, n;
	int row = 8;
	event.data = NULL;
	scan_dir("/dev/input");

//	event.type = get_event();
//

	ret = poll(ufds, nfds, 500);
	if(ret > 0)
	{
		for (i = 0; i < nfds; i++){
			if (ufds[i].revents & POLLIN) {
				n = read(ufds[i].fd, &input, sizeof(input));
				if (n >= sizeof(input)) {
					ret = proc_event(&input);
					if (ret == KEY_VOLUMEUP ||
							ret == KEY_VOLUMEDOWN ||
							ret == KEY_POWER) {
						return ret;
					}
				}
			}
		}
	}
	else
		return 0;
}
void event_init(void)
{
	pthread_t p;

	nfds = 0;
	ufds = NULL;
	scan_dir("/dev/input");

	pthread_create(&p, NULL, key_event_proc, NULL);
}

 int proc_event(struct input_event *event)
{
	int i;
	int code;

	switch(event->type) {
		case EV_KEY:
			if (event->value == 0)
				return event->code;
		default:
			return -1;
	}
}

static int get_event(void)
{
	int i;
	struct input_event event;
	int ret, n;
	int row = 8;

	while (1) {
		poll(ufds, nfds, 100);
		for (i = 0; i < nfds; i++){
			if (ufds[i].revents & POLLIN) {
				n = read(ufds[i].fd, &event, sizeof(event));
				if (n >= sizeof(event)) {
					ret = proc_event(&event);
					if (ret == KEY_VOLUMEUP ||
							ret == KEY_VOLUMEDOWN ||
							ret == KEY_POWER) {
						return ret;
					}
				}
			}
		}
	}
}


void process_event(char* buffer, int len)
{
	char* p;
	const char* head = "*#cmd#*";
	enum ev_type type = NO_EV;
	struct event event;

	if (buffer[len -1] != 0)
		return;

	/* now this is a safe string */
	p = strstr(buffer, head);

	if (p == NULL) {
		/* check simple cmd mode */
		if (strcmp(buffer, "u\n") == 0)
                        type = HOST_EV_UP;
                else if (strcmp(buffer, "d\n") == 0)
                        type = HOST_EV_DOWN;
                else if (strcmp(buffer, "e\n") == 0)
                        type = HOST_EV_ENTER;

	} else {
		p += strlen(head);

		if (strcmp(p, "up\n") == 0)
			type = HOST_EV_UP;
		else if (strcmp(p, "down\n") == 0)
			type = HOST_EV_DOWN;
		else if (strcmp(p, "enter\n") == 0)
			type = HOST_EV_ENTER;
	}

	event.type = type;
	event.data = NULL;
	enqueue_event_locked(event);

	return;
}
