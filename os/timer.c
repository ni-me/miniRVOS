#include "os.h"

extern task_queue task_queue_head;
extern void schedule(void);
extern void switch_to_os(void);


/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

static uint32_t _tick = 0;

#define MAX_TIMER 10
static struct timer timer_list[MAX_TIMER];

static struct spinlock *timer_lock = NULL;


/* load timer interval(in ticks) for next timer interrupt.*/
void timer_load(int interval)
{
	/* each CPU has a separate source of timer interrupts. */
	int id = r_mhartid();
	
	*(uint64_t*)CLINT_MTIMECMP(id) = *(uint64_t*)CLINT_MTIME + interval;
}

void timer_init()
{
	struct timer *t = &(timer_list[0]);
	for (int i = 0; i < MAX_TIMER; i++) {
		t->func = NULL; /* use .func to flag if the item is used */
		t->arg = NULL;
		t++;
	}
	timer_lock = (struct spinlock *) malloc(sizeof(struct spinlock));
	if (timer_lock == NULL) {
		return;
	}
	initlock(timer_lock);

	/*
	 * On reset, mtime is cleared to zero, but the mtimecmp registers 
	 * are not reset. So we have to init the mtimecmp manually.
	 */
	timer_load(TIMER_INTERVAL);

	/* enable machine-mode timer interrupts. */
	w_mie(r_mie() | MIE_MTIE);
}


struct timer *timer_create(void (*handler)(void *arg), void *arg, uint32_t timeout)
{
	/* TBD: params should be checked more, but now we just simplify this */
	if (NULL == handler || 0 == timeout) {
		return NULL;
	}

	/* use lock to protect the shared timer_list between multiple tasks */
	spin_lock(timer_lock);

	struct timer *t = &(timer_list[0]);
	int i = 0;
	while (i < MAX_TIMER) {
		if (NULL == t->func) {
			break;
		}
		t++;
		i++;
	}
	if (i >= MAX_TIMER) {
		spin_unlock(timer_lock);
		return NULL;
	}

	t->func = handler;
	t->arg = arg;
	t->timeout_tick = _tick + timeout;

	spin_unlock(timer_lock);

	return t;
}

void timer_delete(struct timer *timer)
{
	spin_lock(timer_lock);

	struct timer *t = &(timer_list[0]);
	for (int i = 0; i < MAX_TIMER; i++) {
		if (t == timer) {
			t->func = NULL;
			t->arg = NULL;
			break;
		}
		t++;
	}

	spin_unlock(timer_lock);
}

/* this routine should be called in interrupt context (interrupt is disabled) */
static inline void timer_check()
{
	struct timer *t = &(timer_list[0]);
	for (int i = 0; i < MAX_TIMER; i++) {
		if (NULL != t->func) {
			if (_tick >= t->timeout_tick) {
				t->func(t->arg);

				/* once time, just delete it after timeout */
				t->func = NULL;
				t->arg = NULL;

				break;
			}
		}
		t++;
	}
}

void timer_handler() 
{
	_tick++;
	printf("tick: %d\n", _tick);
	
	timer_check();
	timer_load(TIMER_INTERVAL);

	task_resource *t = task_queue_head.next->head->link;
	t->tick++;

	if (t->tick < t->timeslice) {
		switch_to_os();
	} else {
		t->tick = 0;
		task_os();
	}
}
