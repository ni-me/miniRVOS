#include "os.h"

#define MAX_TASKS 10
#define STACK_SIZE 1024
#define PRIORITY_NUMS 256

#define TAKEN 1
#define CLEAR 0

#define _task_current task_queue[priority].current
#define _task_top task_queue[priority].top
#define _tasks task_queue[priority].tasks

uint8_t task_stack[MAX_TASKS][STACK_SIZE];
static int stack_index[MAX_TASKS];


struct queue {
	int current;
	int top;
	struct context tasks[MAX_TASKS];
};

struct task_info {
	reg_t id;
	int stack;
	uint8_t priority;
};

static int task_nums = 0;
static int priority = -1;

struct context ctx_os;
struct context *ctx_current;

struct queue task_queue[PRIORITY_NUMS];
struct task_info info[MAX_TASKS];


static void tasks_init(struct queue *task_queue) {
	for (int i = 0; i < PRIORITY_NUMS; i++) {
		task_queue[i].current = -1;
		task_queue[i].top = 0;
	}
}



/*
 * _top is used to mark the max available position of ctx_tasks
 * _current is used to point to the context of current task
 */


static void w_mscratch(reg_t x)
{
	asm volatile("csrw mscratch, %0" : : "r" (x));
}


static int empty_stack(int idx[]) {
	for (int i = 0; i < MAX_TASKS; i++) {
		if (idx[i] == CLEAR) {
			return i;
		}
	}
	return -1;
}


static inline void set_stack_taken(int i) {
	stack_index[i] = TAKEN;
}

static inline void set_stack_clear(int i) {
	stack_index[i] = CLEAR;
}

static void set_info(int stack, uint8_t priority, int top) {
		info[task_nums].id = (reg_t) &(task_queue[priority].tasks[top]);
		info[task_nums].priority = priority;
		info[task_nums].stack = stack;
}


void sched_init()
{
	w_mscratch(-1);
	tasks_init(task_queue);
}

static int get_current_priority() {
	int res = -1;
	for (int i = 0; i < PRIORITY_NUMS; i++) {
		if (task_queue[i].top > 0) {
			res = i;
			break;
		}
	}
	return res;
}

/*
 * implment a simple cycle FIFO schedular based on priority
 */


struct context *get_next_task()
{
	if (task_nums <= 0) {
		panic("Num of task should be greater than zero!");
		return NULL;
	}

	priority = get_current_priority();
	_task_current = (_task_current + 1) % _task_top;
	struct context *next = &(_tasks[_task_current]);
	/*
	 * switch_to(next);
	 */
	return next;
}


int task_create(void(*task)(void *), void *param, uint8_t priority) {
	
	if (task_nums >= MAX_TASKS) {
		return -1;
	}

	int top = _task_top;
	_tasks[top].a0 = (reg_t) param;
	_tasks[top].ra = (reg_t) task;

	int i = empty_stack(stack_index);

	set_stack_taken(i);
	_tasks[top].sp = (reg_t) &(task_stack[i][STACK_SIZE]);
	set_info(i, priority, top);

	_task_top++;
	task_nums++;

	return 0;
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

static int find_task_info(struct task_info info[], reg_t id) {
	int i;
	for (i = 0; i < task_nums; i++) {
		if (info[i].id == id) {
			break;
		}
	}
	return i;
}


static void move_task_info(struct task_info info[], int first, int last) {
	for (int i = first; i < last - 1; i++) {
		info[i] = info[i + 1];
	}
}


static inline void assgin(struct context *des, struct context *sou) {
	reg_t *t1 = (reg_t *)des;
	reg_t *t2 = (reg_t *)sou;

	/*
	 * The number of registers is 31 (igonre x0 (zero))
	 */

	for (int i = 0; i < 31; i++) {
		*t1++ = *t2++;
	}
}

static void change_id(reg_t id, reg_t des) {
	int i = find_task_info(info, des);
	info[i].id = id;
}

static void move_tasks(struct context tasks[], int first, int last) {
		for (int i = first; i < last - 1; i++) {
		assgin(&tasks[i], &tasks[i + 1]);
		change_id(&tasks[i], &tasks[i + 1]);
	}
}


void task_exit() {
	reg_t id = r_mscratch();
	int pos = find_task_info(info, id);
	struct task_info t = info[pos];

	reg_t priority = t.priority;

	move_tasks(_tasks, _task_current, _task_top);
	move_task_info(info, pos, task_nums);
	
	set_stack_clear(t.stack);

	_task_current--;
	_task_top--;
	task_nums--;

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
	struct context *ctx = ctx_current;
	ctx_current = &ctx_os;
	sys_switch(ctx, ctx_current);
}


void task_go() {
	ctx_current = get_next_task();
	sys_switch(&ctx_os, ctx_current);
}

void os_kernel() {
	task_os();
}
