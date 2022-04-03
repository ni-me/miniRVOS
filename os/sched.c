#include "os.h"

#define STACK_SIZE 1024
#define PRIORITY_NUMS 256


struct task_resource {
	struct task_resource *link;
	struct context task_context;
	uint8_t task_stack[STACK_SIZE];

	int id;
};

struct task_queue {
	uint8_t priority;
	int counter;
	struct task_queue *next;
	struct task_resource *head;
	struct task_resource *tail;
};

struct task_queue task_queue_head;


struct context ctx_os;
struct context *ctx_current;



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

struct task_resource *dequeue(struct task_queue *queue) {
	if (queue->counter == 0) {
		return NULL;
	}

	struct task_resource *res = queue->head->link;
	queue->head->link = res->link;

	if (queue->head->link == NULL) {
		queue->tail = queue->head;
	}

	queue->counter--;
	res->link = NULL;

	return res;
}


void enqueue(struct task_queue *queue, struct task_resource *task) {
	queue->tail->link = task;
	queue->tail = task;
	queue->tail->link = NULL;
	
	queue->counter++;
}



struct context *get_next_task()
{
	struct task_queue *ptr = task_queue_head.next;

	if (ptr != NULL) {
		struct task_resource *t = dequeue(ptr);
		enqueue(ptr, t);
		return &t->task_context;
	}
	return NULL;
}


struct task_queue *find_task_queue(uint8_t priority) {
	struct task_queue *t = task_queue_head.next;
	struct task_queue *res = NULL;

	while (t != NULL && t->priority <= priority) {
		if (t->priority == priority) {
			res = t;
			break;
		}
		t = t->next;
	}

	return res;
}

struct task_queue *add_task_queue(uint8_t priority) {
	struct task_queue *res = find_task_queue(priority);

	if (res == NULL) {
		struct task_queue *curr = task_queue_head.next;
		struct task_queue *pre = &task_queue_head;

		while (curr != NULL && curr->priority < priority) {
			curr = curr->next;
			pre = pre->next;
		}

		res = (struct task_queue *) malloc(sizeof(struct task_queue));
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

int task_create(void(*task)(void *), void *param, uint8_t priority, int id) {
	if (task == NULL) {
		return -1;
	}

	struct task_resource *new_task = (struct task_resource *) malloc(sizeof(struct task_resource));
	if (new_task == NULL) {
		return -1;
	}

	new_task->task_context.ra = (reg_t) task;
	new_task->task_context.a0 = (reg_t) param;
	new_task->task_context.sp = (reg_t) &new_task->task_stack[STACK_SIZE];
	new_task->id = id;
	new_task->link = NULL;

	struct task_queue *task_queue_ptr = find_task_queue(priority);
	struct task_resource *task_resource_ptr = NULL;

	/*
	 * task queue has not existed
	 */
	if (task_queue_ptr == NULL) {
		task_queue_ptr = add_task_queue(priority);
		if (task_queue_ptr == NULL) {
			return -1;
		}

		/* add task list head */
		task_resource_ptr = (struct task_resource *) malloc(sizeof(struct task_resource));
		if (task_resource_ptr == NULL) {
			return -1;
		}
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
}



void display_tasks() {
	struct task_queue *queue = task_queue_head.next;
	struct task_resource *task;

	while (queue != NULL) {
		printf("Priority %d:\n", queue->priority);
		task = queue->head->link;

		while (task != NULL) {
			printf("%d ", task->id);
			task = task->link;
		}
		printf("\n");
		queue = queue->next;
	}
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
	struct context *ctx = ctx_current;
	ctx_current = &ctx_os;
	/* switch to os */
	sys_switch(ctx, &ctx_os);
}


void task_go() {
	ctx_current = get_next_task();
	/* switch to user task */
	sys_switch(&ctx_os, ctx_current);
}

void os_kernel() {
	task_os();
}