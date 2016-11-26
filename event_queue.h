//#include "event.h"

struct event;

extern void event_queue_init_need(unsigned int n);
extern void event_queue_uninit(void);
extern int dequeue_event_locked(struct event* out);
extern int enqueue_event_locked(struct event in);


