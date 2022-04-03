#ifndef __OS_H__
#define __OS_H__

#include "types.h"
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
struct context {
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
};

extern void sys_switch(struct context *ctx_old, struct context *ctx_new);

extern int  task_create(void (*task)(void *param), void *param, uint8_t priority);
extern void task_delay(volatile int count);
extern void task_exit();
extern void os_kernel();
extern void task_go();

#endif /* __OS_H__ */
