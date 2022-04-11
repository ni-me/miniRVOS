#include "os.h"

/*
 * Following functions SHOULD be called ONLY ONE time here,
 * so just declared here ONCE and NOT included in file os.h.
 */
extern void uart_init();
extern void page_init();
extern void sched_init();
extern void schedule();
extern void os_main();
extern void trap_init();
extern void plic_init();
extern void timer_init();

// extern void display_timer(void);
// extern void display_delay(void);
// extern void display_activate(void);

char buffer[BUFFER_LENGTH];

void start_kernel(void)
{
	uart_init();
	uart_puts("Hello, RVOS!\n");
	
	page_init();

	trap_init();

	plic_init();

	timer_init();

	sched_init();

	os_main();

	while (1) {
		 // display_activate();
		 // display_delay();
		 // display_timer();	
		 uart_puts("OS: Activate next task\n");
		 task_go();
		 uart_puts("OS: Back to OS\n\n");
	} // stop here!
}

