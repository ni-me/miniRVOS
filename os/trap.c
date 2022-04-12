#include "os.h"

extern void trap_vector();
extern void uart_isr();
extern void timer_handler();
extern void destory();
extern void switch_to_os();
extern void activate();
extern void delay(uint32_t tick);
extern void do_syscall(struct context *cxt);
extern int gethid(unsigned int *hid);

extern task_queue *task_queue_head;

void trap_init()
{
	/*
	 * set the trap-vector base-address for machine-mode
	 */
	w_mtvec((reg_t)trap_vector);
}

void software_trigger(reg_t code, uint32_t tick)
{
	/* When jump into trap_vector(), $a0 is equal to code, $a1 is equal to tick*/

	// int id = r_mhartid();
	// *(uint32_t*)CLINT_MSIP(id) = 1;
	unsigned int hid;
	int ret = -1;
	ret = gethid(&hid);

	if (ret == 0) {
		/* restore $a0 and $a1 */
		asm volatile (
			"mv a0, %[input0]\n"
			"mv a1, %[input1]\n"
			:
			:[input0]"r"(code), [input1]"r"(tick)
	);
		*(uint32_t*)CLINT_MSIP(hid) = 1;
	} else {
		panic("OPPS! software_trigger: error\n");
	}

}


void external_interrupt_handler()
{
	int irq = plic_claim();

	switch (irq) {
		case 0:
			break;
		case UART0_IRQ:
			uart_isr();
			break;
		default:
			printf("unexpected interrupt irq = %d\n", irq);
			break;
	}

	if (irq) {
		plic_complete(irq);
	}
}


void software_interrupt_handler(reg_t code, uint32_t tick) {
	/*
	 * acknowledge the software interrupt by clearing
     * the MSIP bit in mip.
	 */
	int id = r_mhartid();
	*(uint32_t*)CLINT_MSIP(id) = 0;

	switch(code) {
		case TASK_YEILD_CODE: {
			task_resource *t = task_queue_head->next->head->link;
			t->tick = 0;
			task_os();
			break;
		}
		case TASK_EXIT_CODE:
			destory();
			switch_to_os();
			break;
		case TASK_DELAY_CODE:
			timer_create(activate, NULL, tick);
			delay(tick);
			break;
		default:
			break;
	}
}


reg_t trap_handler(reg_t code, uint32_t tick, reg_t epc, reg_t cause, context *cxt)
{
	reg_t return_pc = epc;
	reg_t cause_code = cause & 0xfff;
	
	if (cause & 0x80000000) {
		/* Asynchronous trap - interrupt */
		switch (cause_code) {
		case 3:
			uart_puts("software interruption!\n");
			software_interrupt_handler(code, tick);
			break;
		case 7:
			uart_puts("timer interruption!\n");
			timer_handler();
			break;
		case 11:
			uart_puts("external interruption!\n");
			external_interrupt_handler();
			break;
		default:
			uart_puts("unknown async exception!\n");
			break;
		}
	} else {
		/* Synchronous trap - exception */
		printf("Sync exceptions!, code = %d\n", cause_code);
		switch (cause_code) {
		case 8:
			uart_puts("System call from U-mode!\n");
			do_syscall(cxt);
			return_pc += 4;
			break;
		default:
			panic("OOPS! What can I do!");
			//return_pc += 4;
		}
	}
	return return_pc;
}

void trap_test()
{
	/*
	 * Synchronous exception code = 7
	 * Store/AMO access fault
	 */
	*(int *)0x00000000 = 100;

	/*
	 * Synchronous exception code = 5
	 * Load access fault
	 */
	//int a = *(int *)0x00000000;

	uart_puts("Yeah! I'm return back from trap!\n");
}

