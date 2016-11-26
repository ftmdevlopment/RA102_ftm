#include "../../event_queue.h"
#include "../../event.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>


void* event_producer(void* arg)
{
	int i = 1024;
	char* c = (char*)arg;
	while (i--) {
		struct event ev = {
			.type = KEY_EV_UP,
		};

		ev.data = (unsigned char*)malloc(30);
		//ev.data[0] = c[0];
		//ev.data[1] = '-';
		printf("thread %c enqueue\n", c[0]);
		sprintf(&ev.data[0], "%c-%d\0",c[0], i);
		enqueue_event_locked(ev);
		usleep(50);
	}

	return NULL;
}

int main()
{
	pthread_t p0,p1,p2,p3,p4,p5;
	int ret;
	void* res;

#if 0
	/* test enqueue */
	{
		int i = 0;
		event_queue_init_need(4);
		while(i++ < 5){
			struct event ev = {
				.type = KEY_EV_UP,
			};

			ev.data = (unsigned char*)malloc(10);
			sprintf(&ev.data[0], "%d\0", i);
			enqueue_event_locked(ev);
		}

		printf("enqueue ok\n");
		event_queue_uninit();
		return 1;
	}
#endif

	event_queue_init_need(1024);
	ret = pthread_create(&p0, NULL, event_producer, "0");
	ret = pthread_create(&p1, NULL, event_producer, "1");
	ret = pthread_create(&p2, NULL, event_producer, "2");
	ret = pthread_create(&p3, NULL, event_producer, "3");
	ret = pthread_create(&p4, NULL, event_producer, "4");
	ret = pthread_create(&p5, NULL, event_producer, "5");


	while (1) {
		struct event ev;
		dequeue_event_locked(&ev);
		if (ev.data != NULL) {
			printf("dequeue: %s\n", ev.data);
			free(ev.data);
			ev.data = NULL;
		}
		usleep(50);
	}

	event_queue_uninit();

	return 0;
}
