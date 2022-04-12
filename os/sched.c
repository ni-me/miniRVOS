#include "os.h"

#define STACK_SIZE 1024

task_queue *task_queue_head;
struct delay_list *delay_list_head;

static task_queue tqhead;
static struct delay_list dhead;

static context ctx_os;
static context *ctx_current;

extern uint32_t _tick;

extern void software_trigger(reg_t code, uint32_t tick);

/* a very rough implementaion, just to consume the cpu */
void wait(volatile int count)
{
	count *= 50000;
	while (count--);
}


static void idle_task() {
	while (1) {
		// waiting for ~= 1 s
		wait(DELAY);
	}
}


static void tasks_init()
{
	task_queue_head = &tqhead;
	delay_list_head = &dhead;

	task_queue_head->next = NULL;
	delay_list_head->next = NULL;

	task_create(idle_task, NULL, 255, 1);
}


static inline task_resource *dequeue(task_queue *queue)
{
	if (queue == NULL) {
		return NULL;
	}

	task_resource *res = queue->head->link;
	queue->head->link = res->link;

	if (queue->head->link == NULL) {
		queue->tail = queue->head;
	}

	queue->counter--;
	res->link = NULL;

	return res;
}


static inline void enqueue(task_queue *queue, task_resource *task)
{
	queue->tail->link = task;
	queue->tail = task;
	queue->tail->link = NULL;
	
	queue->counter++;
}


static inline context *get_next_task()
{
	task_queue *ptr = task_queue_head->next;

	if (ptr != NULL) {
		return ptr->head->link->task_context;
	}
	return NULL;
}


static inline task_queue *find_task_queue(uint8_t priority)
{
	task_queue *t = task_queue_head->next;

	while (t != NULL && t->priority <= priority) {
		if (t->priority == priority) {
			return t;
		}
		t = t->next;
	}

	return NULL;
}

static inline task_queue *new_task_queue(uint8_t priority)
{
	task_queue *node = (task_queue *) malloc(sizeof(task_queue));

	if (node == NULL) {
		return NULL;
	}

	node->counter = 0;
	node->priority = priority;
	node->head = NULL;
	node->tail = NULL;
	node->next = NULL;
		
	return node;
}


static inline task_resource *new_task_resource()
{
	task_resource *ptr = (task_resource *) malloc(sizeof(task_resource));
	if (ptr == NULL) {
		return NULL;
	}
	ptr->task_context = NULL;
	ptr->task_stack = NULL;
	ptr->link = NULL;

	return ptr;
}


static inline task_queue *add_task_queue(uint8_t priority)
{
	task_queue *res = find_task_queue(priority);

	if (res == NULL) {
		task_queue *curr = task_queue_head->next;
		task_queue *pre = task_queue_head;

		while (curr != NULL && curr->priority < priority) {
			curr = curr->next;
			pre = pre->next;
		}

		res = new_task_queue(priority);
		if (res != NULL) {
			res->next = curr;
			pre->next = res;
		}
	}

	return res;
}


static inline void task_insert(uint8_t priority, task_resource *task) {
		task_queue *task_queue_ptr = find_task_queue(priority);
		task_resource *task_resource_ptr = NULL;

		/*
		* task queue has not existed
		*/
		if (task_queue_ptr == NULL) {
			task_queue_ptr = add_task_queue(priority);
			if (task_queue_ptr == NULL) {
				return;
			}

			/* add task list head */
			task_resource_ptr = new_task_resource();
			if (task_resource_ptr == NULL) {
				return;
			}

			task_queue_ptr->head = task_resource_ptr;
			task_queue_ptr->tail = task_resource_ptr;
		}

		enqueue(task_queue_ptr, task);
}


void switch_to_os() {
	ctx_current = &ctx_os;
	switch_to(&ctx_os);
}

void task_create(void(*task)(void *), void *param, uint8_t priority, uint32_t timeslice)
{
	if (task == NULL) {
		return;
	}

	task_resource *new_task = (task_resource *) malloc(sizeof(task_resource));
	new_task->task_context = (context *) malloc(sizeof(context));
	new_task->task_stack = (uint8_t *) malloc(sizeof(uint8_t) * STACK_SIZE);

	if (!new_task || !new_task->task_context || !new_task->task_stack) {
		return;
	}

	new_task->tick = 0;
	new_task->timeslice = timeslice;
	new_task->priority = priority;
	new_task->task_context->pc = (reg_t) task;
	new_task->task_context->a0 = (reg_t) param;
	new_task->task_context->sp = (reg_t) (new_task->task_stack + STACK_SIZE);
	new_task->link = NULL;

	task_insert(priority, new_task);
}


void destory()
{
	task_queue *queue = task_queue_head->next;
	if (queue == NULL) {
		return;
	}

	/* free task_resource */
	task_resource *task = dequeue(queue);
	free(task->task_context);
	free(task->task_stack);
	free(task);

	if (queue->counter == 0) {
		task_queue_head->next = queue->next;
		queue->next = NULL;

		free(queue->head);
		free(queue);
	}
}


void activate() {
	struct delay_list *pre = delay_list_head;
	struct delay_list *t = delay_list_head->next;

	while (t != NULL && t->timeout <= _tick) {
		task_insert(t->task->priority, t->task);

		struct delay_list *tmp = t;

		t = t->next;
		
		pre->next = t;
		tmp->next = NULL;
		free(tmp);
	}
}

static void delay_list_insert(struct delay_list *node) {
	struct delay_list *pre = delay_list_head;
	struct delay_list *curr = delay_list_head->next;

	while (curr != NULL && curr->timeout < node->timeout) {
		pre = pre->next;
		curr = curr->next;
	}
	
	node->next = curr;
	pre->next = node;
}


void delay(uint32_t tick) {
	task_queue *queue = task_queue_head->next;
	if (queue == NULL) {
		return;
	}

	struct delay_list *t = (struct delay_list *) malloc(sizeof(struct delay_list));
	if (t == NULL) {
		return;
	}

	task_resource *task = dequeue(queue);

	t->task = task;
	t->timeout = tick + _tick;
	t->next = NULL;

	if (queue->counter == 0) {
		task_queue_head->next = queue->next;
		queue->next = NULL;

		free(queue->head);
		free(queue);
	}

	delay_list_insert(t);
	switch_to_os();
}


void sched_init()
{
	w_mscratch(0);
	tasks_init();
	/* enable machine-mode software interrupts. */
	w_mie(r_mie() | MIE_MSIE);
}


void task_delay(uint32_t tick)
{
	software_trigger(TASK_DELAY_CODE, tick);
}


void task_exit()
{
	/* trigger a machine-level software interrupt */
	software_trigger(TASK_EXIT_CODE, 0);
}


void task_os()
{
	task_resource *old_task = dequeue(task_queue_head->next);
	enqueue(task_queue_head->next, old_task);

	switch_to_os();
}


void task_go()
{
	ctx_current = get_next_task();
	if (ctx_current == NULL) {
		panic("OPPS! There is no user task running on OS now");
	}
	/* switch to user task */
	sys_switch(&ctx_os, ctx_current);
}


void task_yeild()
{
	/* trigger a machine-level software interrupt */
	software_trigger(TASK_YEILD_CODE, 0);
}
