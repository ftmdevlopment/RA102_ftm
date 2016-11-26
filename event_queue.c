#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include "event.h"
#include "console.h"

#define DEBUG 0

struct event_queue {
	int n; /* buffer length, element num */
	int p; /* n = 2^p for performance */
	int queued; /* event queued in the queue */
	int dequeue_index;
	int inited;
	int enqueue_index;
	struct event* buffer;
	sem_t sem;
	sem_t dq_sem;
} event_queue = {
	.inited = 0,
};

static void update_index(int flag);

void free_event(struct event* ev)
{
	ev->type = NO_EV;
	if (ev->data != NULL) {
		free(ev->data);
		ev->data = NULL;
	}
}

void event_queue_init_need(unsigned int n)
{
	int a = 0, b = 0;
	b = a = 0x1 << 10;

	while (a > n) {
		a >>= 1;
		b = a;
	}

	event_queue.n = b;
	event_queue.queued = 0;
	event_queue.buffer = (struct event*)malloc(sizeof(struct event) * event_queue.n);
	memset(event_queue.buffer, 0, sizeof(struct event*) * event_queue.n);
	event_queue.dequeue_index = 0;
	event_queue.enqueue_index = 0;
	sem_init(&event_queue.sem, 0, 1);
	sem_init(&event_queue.dq_sem, 0, 0);

	event_queue.inited = 1; /* now the queue is inited */
}

void event_queue_uninit(void)
{
	int i;


	/* free all queued, for some event may hold private data */
	if (event_queue.queued != 0) {
		int i = event_queue.queued;
		while (i > 0) {
			struct event ev;
			dequeue_event_locked(&ev);
			if (ev.data != NULL) {
				free(ev.data);
				ev.data = NULL;
			}
			i--;
		}
	}
	free(event_queue.buffer);

	/* set the flag to 0, so all the dequeue and enqueue can not works */
	event_queue.inited = 0;
}

static void update_index(int flag)
{
	if (flag == -1) {
		/* dequeue */
		struct event* ev = &event_queue.buffer[event_queue.dequeue_index];
		/* un-init */
		ev->type = NO_EV;
		/* update index */
		event_queue.dequeue_index++;
		event_queue.dequeue_index &= (event_queue.n-1);
		event_queue.queued--;
	} else if (flag == 1) {
		/* enqueue */
		event_queue.enqueue_index++;
		event_queue.enqueue_index &= (event_queue.n-1);
		event_queue.queued++;
	}
}


int dequeue_event_locked(struct event* out)
{
	if (event_queue.inited != 1)
		return 1;
//++ add engmode log
/*
	if (DEBUG)
		print("dequeue\n");
*/
	if (DEBUG){
		if (LOG_PRINT_PLACE){
			print("dequeue\n");
		}
		else{
			ALOGD("dequeue\n");
		}
	}
	sem_wait(&event_queue.dq_sem); /* dec */
/*
	if (DEBUG)
		print("have event to dequeue\n");
*/
	if (DEBUG){
		if (LOG_PRINT_PLACE){
			print("have event to dequeue\n");
		}
		else{
			ALOGD("have event to dequeue\n");
		}
	}
	sem_wait(&event_queue.sem);
	if (event_queue.queued > 0) {

		if (DEBUG){
			if (LOG_PRINT_PLACE){
				print("dequeue: still %d events in the queue\n", event_queue.queued);
				print("dequeue: enqueue_index = %d\n", event_queue.enqueue_index);
				print("dequeue: dequeue_index = %d\n", event_queue.dequeue_index);
			}
			else{
				ALOGD("dequeue: still %d events in the queue\n", event_queue.queued);
				ALOGD("dequeue: enqueue_index = %d\n", event_queue.enqueue_index);
				ALOGD("dequeue: dequeue_index = %d\n", event_queue.dequeue_index);
			}
		}

		*out = event_queue.buffer[event_queue.dequeue_index];
		update_index(-1);

		if (DEBUG){
			if (LOG_PRINT_PLACE){
				print("dequeue: ok\n");
			}
			else{
				ALOGD("dequeue: ok\n");
			}
		}
//-- add engmode log
		sem_post(&event_queue.sem);
		return 0;
	}

	sem_post(&event_queue.sem);
	return 1;
}

int enqueue_event_locked(struct event in)
{
//++ add engmode log
	if (DEBUG){
		if (LOG_PRINT_PLACE){
			print("enqueue: in\n");
		}
		else{
			ALOGD("enqueue: in\n");
		}
	}

	if (event_queue.inited != 1)
		return 1;

	if (DEBUG){
		if (LOG_PRINT_PLACE){
			print("enqueue: inited\n");
		}
		else{
			ALOGD("enqueue: inited\n");
		}
	}
	sem_wait(&event_queue.sem);

	if (DEBUG){
		if (LOG_PRINT_PLACE){
			print("enqueue_index = %d\n", event_queue.enqueue_index);
			print("dequeue_index = %d\n", event_queue.dequeue_index);
		}
		else{
			ALOGD("enqueue_index = %d\n", event_queue.enqueue_index);
			ALOGD("dequeue_index = %d\n", event_queue.dequeue_index);
		}
	}

	/* overflow */
	if (event_queue.enqueue_index == event_queue.dequeue_index
			&& event_queue.queued != 0) {
		if (DEBUG){
			if (LOG_PRINT_PLACE){
				print("overflow\n");
			}
			else{
				ALOGD("overflow\n");
			}
		}
		free_event(&event_queue.buffer[event_queue.dequeue_index]);
		update_index(-1);
	}
	event_queue.buffer[event_queue.enqueue_index] = in;
	update_index(1);
	sem_post(&event_queue.sem);
	sem_post(&event_queue.dq_sem); /* add */

	if (DEBUG){
		if (LOG_PRINT_PLACE){
			print("enqueue ok\n");
		}
		else{
			ALOGD("enqueue ok\n");
		}
	}
//-- add engmode log
	return 0;
}
