#include "os.h"

extern void trap_vector(void);
extern void uart_isr(void);
extern void timer_handler(void);
extern void destory(void);
extern void switch_to_os(void);

extern task_queue task_queue_head;

void trap_init()
{
	/*
	 * set the trap-vector base-address for machine-mode
	 */
	w_mtvec((reg_t)trap_vector);
}

void software_trigger(reg_t code)
{
	/* When jump into trap_vector(), $a0 is equal to code */

	int id = r_mhartid();
	*(uint32_t*)CLINT_MSIP(id) = 1;
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


void software_interrupt_handler(reg_t code) {
	/*
	 * acknowledge the software interrupt by clearing
     * the MSIP bit in mip.
	 */
	int id = r_mhartid();
	*(uint32_t*)CLINT_MSIP(id) = 0;

	switch(code) {
		case TASK_YEILD_CODE: {
			task_resource *t = task_queue_head.next->head->link;
			t->tick = 0;
			task_os();
			break;
		}
		case TASK_EXIT_CODE:
			destory();
			switch_to_os();
			break;
		case TASK_DELAY_CODE:
			break;
		default:
			break;
	}
}


reg_t trap_handler(reg_t code, reg_t epc, reg_t cause)
{
	reg_t return_pc = epc;
	reg_t cause_code = cause & 0xfff;
	
	if (cause & 0x80000000) {
		/* Asynchronous trap - interrupt */
		switch (cause_code) {
		case 3:
			uart_puts("software interruption!\n");
			software_interrupt_handler(code);
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
		panic("OOPS! What can I do!");
		//return_pc += 4;
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

