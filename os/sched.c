#include "os.h"

#define STACK_SIZE 1024

task_queue task_queue_head;

context ctx_os;
context *ctx_current;

static void tasks_init() {
	task_queue_head.counter = 0;
	task_queue_head.priority = 0;
	task_queue_head.next = NULL;
	task_queue_head.head = NULL;
	task_queue_head.tail = NULL;
}

static void w_mscratch(reg_t x)
{
	asm volatile("csrw mscratch, %0" : : "r" (x));
}

static reg_t r_mscratch() {
	reg_t val = 0;
	asm volatile (
		"csrrw t0, mscratch, t0\n"
		"mv %0, t0\n"
		"csrrw t0, mscratch, t0\n"
		:"=r"(val)
	);

	return val;
}

void sched_init()
{
	w_mscratch(-1);
	tasks_init();
}

/*
 * implment a simple cycle FIFO schedular based on priority
 */

task_resource *dequeue(task_queue *queue) {
	if (queue == NULL || queue->counter == 0) {
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


void enqueue(task_queue *queue, task_resource *task) {
	queue->tail->link = task;
	queue->tail = task;
	queue->tail->link = NULL;
	
	queue->counter++;
}



context *get_next_task()
{
	task_queue *ptr = task_queue_head.next;

	if (ptr != NULL) {
		return ptr->head->link->task_context;
	}
	return NULL;
}


task_queue *find_task_queue(uint8_t priority) {
	task_queue *t = task_queue_head.next;
	task_queue *res = NULL;

	while (t != NULL && t->priority <= priority) {
		if (t->priority == priority) {
			res = t;
			break;
		}
		t = t->next;
	}

	return res;
}

task_queue *add_task_queue(uint8_t priority) {
	task_queue *res = find_task_queue(priority);

	if (res == NULL) {
		task_queue *curr = task_queue_head.next;
		task_queue *pre = &task_queue_head;

		while (curr != NULL && curr->priority < priority) {
			curr = curr->next;
			pre = pre->next;
		}

		res = (task_queue *) malloc(sizeof(task_queue));
		if (res == NULL) {
			return NULL;
		}

		res->counter = 0;
		res->priority = priority;
		res->head = NULL;
		res->tail = NULL;
		

		res->next = curr;
		pre->next = res;

	}

	return res;
}

int task_create(void(*task)(void *), void *param, uint8_t priority) {
	if (task == NULL) {
		return -1;
	}

	task_resource *new_task = (task_resource *) malloc(sizeof(task_resource));
	new_task->task_context = (context *) malloc(sizeof(context));
	new_task->task_stack = (uint8_t *) malloc(sizeof(uint8_t) * STACK_SIZE);

	if (!new_task || !new_task->task_context || !new_task->task_stack) {
		return -1;
	}

	new_task->task_context->ra = (reg_t) task;
	new_task->task_context->a0 = (reg_t) param;
	new_task->task_context->sp = (reg_t) (new_task->task_stack + STACK_SIZE);
	new_task->link = NULL;

	task_queue *task_queue_ptr = find_task_queue(priority);
	task_resource *task_resource_ptr = NULL;

	/*
	 * task queue has not existed
	 */
	if (task_queue_ptr == NULL) {
		task_queue_ptr = add_task_queue(priority);
		if (task_queue_ptr == NULL) {
			return -1;
		}

		/* add task list head */
		task_resource_ptr = (task_resource *) malloc(sizeof(task_resource));
		if (task_resource_ptr == NULL) {
			return -1;
		}
		task_resource_ptr->task_context = NULL;
		task_resource_ptr->task_stack = NULL;
		task_resource_ptr->link = NULL;

		task_queue_ptr->head = task_resource_ptr;
		task_queue_ptr->tail = task_resource_ptr;
	}

	task_queue_ptr->tail->link = new_task;
	task_queue_ptr->tail = new_task;
	task_queue_ptr->tail->link = NULL;
	task_queue_ptr->counter++;

	return 0;
}


void task_exit() {
	task_queue *queue = task_queue_head.next;
	if (queue == NULL) {
		return;
	}

	task_resource *task = dequeue(queue);
	free(task->task_context);
	free(task->task_stack);
	free(task);

	if (queue->counter == 0) {
		task_queue_head.next = queue->next;
		queue->next = NULL;
		free(queue);
	}

	w_mscratch(0);

	task_os();
}

/*
 * a very rough implementaion, just to consume the cpu
 */
void task_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

void task_os() {
	context *ctx = ctx_current;
	ctx_current = &ctx_os;

	/* switch to os */
	sys_switch(ctx, &ctx_os);
}


void task_go() {
	ctx_current = get_next_task();
	if (ctx_current == NULL) {
		panic("OPPS! There is no user task running on OS now");
	}
	/* switch to user task */
	sys_switch(&ctx_os, ctx_current);
}

void task_yeild() {
	task_resource *old_task = dequeue(task_queue_head.next);
	enqueue(task_queue_head.next, old_task);

	task_os();
}
