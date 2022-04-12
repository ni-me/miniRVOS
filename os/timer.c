#include "os.h"

extern task_queue *task_queue_head;
extern void schedule();
extern void switch_to_os();


/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

uint32_t _tick = 0;

static struct spinlock *timer_lock = NULL;

static struct timer *timer_head = NULL;


/* load timer interval(in ticks) for next timer interrupt.*/
void timer_load(int interval)
{
	/* each CPU has a separate source of timer interrupts. */
	int id = r_mhartid();
	
	*(uint64_t*)CLINT_MTIMECMP(id) = *(uint64_t*)CLINT_MTIME + interval;
}

void timer_init()
{
	timer_lock = (struct spinlock *) malloc(sizeof(struct spinlock));
	if (timer_lock == NULL) {
		return;
	}
	initlock(timer_lock);

	timer_head = (struct timer *) malloc(sizeof(struct timer));
	if (timer_head == NULL) {
		return;
	}
	timer_head->next = NULL;

	/*
	 * On reset, mtime is cleared to zero, but the mtimecmp registers 
	 * are not reset. So we have to init the mtimecmp manually.
	 */
	timer_load(TIMER_INTERVAL);

	/* enable machine-mode timer interrupts. */
	w_mie(r_mie() | MIE_MTIE);
}


static inline void insert(struct timer *timer)
{
	struct timer *pre = timer_head;
	struct timer *curr = timer_head->next;

	while (curr != NULL && curr->timeout_tick < timer->timeout_tick) {
		pre = pre->next;
		curr = curr->next;
	}
	
	timer->next = curr;
	pre->next = timer;
}


struct timer *timer_create(void (*handler)(void *arg), void *arg, uint32_t timeout)
{
	/* TBD: params should be checked more, but now we just simplify this */
	if (NULL == handler || 0 == timeout) {
		return NULL;
	}

	/* use lock to protect the shared timer_list between multiple tasks */
	spin_lock(timer_lock);

	struct timer *t = (struct timer *) malloc(sizeof(struct timer));
	if (t == NULL) {
		return NULL;
	}

	t->func = handler;
	t->arg = arg;
	t->timeout_tick = _tick + timeout;
	insert(t);

	spin_unlock(timer_lock);

	return t;
}


void timer_delete(struct timer *timer)
{
	spin_lock(timer_lock);
	struct timer *pre = timer_head;
	struct timer *curr = timer_head->next;

	while (curr != NULL) {
		if (curr == timer) {
			pre->next = curr->next;
			curr->next = NULL;
			free(curr);
			break;
		}
		pre = pre->next;
		curr = curr->next;
	}
	spin_unlock(timer_lock);
}

/* this routine should be called in interrupt context (interrupt is disabled) */
static inline void timer_check()
{
	struct timer *t = timer_head->next;
	struct timer *tmp;

	while (t != NULL && t->timeout_tick <= _tick) {
		tmp = t;
		t->func(t->arg);
		t = t->next;
		timer_delete(tmp);
	}
}

void timer_handler() 
{
	_tick++;
	printf("tick: %d\n", _tick);
	
	timer_check();
	timer_load(TIMER_INTERVAL);

	task_resource *t = task_queue_head->next->head->link;
	t->tick++;

	if (t->tick < t->timeslice) {
		switch_to_os();
	} else {
		t->tick = 0;
		task_os();
	}
}
