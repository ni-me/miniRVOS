#ifndef __OS_H__
#define __OS_H__

#include "types.h"
#include "riscv.h"
#include "platform.h"

#include <stddef.h>
#include <stdarg.h>

/* uart */
#define BUFFER_LENGTH 100

extern int uart_putc(char ch);
extern void uart_puts(char *s);
extern char uart_getc();
extern int uart_gets(char *s);

/* printf */
extern int  printf(const char* s, ...);
extern void panic(char *s);

/* memory management */
extern void *page_alloc(int npages);
extern void page_free(void *p);
extern void *malloc(size_t size);
extern void free(void *ptr);


/* task management */

#define TASK_YEILD_CODE 0
#define TASK_EXIT_CODE 1
#define TASK_DELAY_CODE 2

#define DELAY 1000

typedef struct context {
	/* ignore x0 */
	reg_t ra;
	reg_t sp;
	reg_t gp;
	reg_t tp;
	reg_t t0;
	reg_t t1;
	reg_t t2;
	reg_t s0;
	reg_t s1;
	reg_t a0;
	reg_t a1;
	reg_t a2;
	reg_t a3;
	reg_t a4;
	reg_t a5;
	reg_t a6;
	reg_t a7;
	reg_t s2;
	reg_t s3;
	reg_t s4;
	reg_t s5;
	reg_t s6;
	reg_t s7;
	reg_t s8;
	reg_t s9;
	reg_t s10;
	reg_t s11;
	reg_t t3;
	reg_t t4;
	reg_t t5;
	reg_t t6;

	// save the pc to run in next schedule cycle
	reg_t pc; // offset: 31 *4 = 124

} context;


typedef struct task_resource {
	uint32_t tick;
	uint32_t timeslice;
	uint8_t priority;
	struct task_resource *link;
	struct context *task_context;
	uint8_t *task_stack;
} task_resource;


typedef struct task_queue {
	uint8_t priority;
	int counter;
	struct task_queue *next;
	struct task_resource *head;
	struct task_resource *tail;
} task_queue;


struct delay_list {
	uint32_t timeout;
	task_resource *task;
	struct delay_list *next;	
};


extern void sys_switch(struct context *ctx_old, struct context *ctx_new);
extern void switch_to(struct context *ctx);

extern void task_create(void (*task)(void *param), void *param, uint8_t priority, uint32_t timeslice);
extern void task_delay(uint32_t tick);
extern void task_exit();
extern void task_yeild();
extern void task_go();
extern void task_os();

extern void wait(volatile int count);

/* plic */
extern int plic_claim(void);
extern void plic_complete(int irq);

/* lock */

struct spinlock {
	int locked;
};

extern void initlock(struct spinlock *);
extern void spin_lock(struct spinlock *);
extern void spin_unlock(struct spinlock *);

/* software timer */
struct timer {
	void (*func)(void *arg);
	void *arg;
	uint32_t timeout_tick;
	struct timer *next;
};
extern struct timer *timer_create(void (*handler)(void *arg), void *arg, uint32_t timeout);
extern void timer_delete(struct timer *timer);
#endif /* __OS_H__ */
