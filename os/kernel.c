#include "os.h"

/*
 * Following functions SHOULD be called ONLY ONE time here,
 * so just declared here ONCE and NOT included in file os.h.
 */
extern void uart_init(void);
extern void page_init(void);
extern void page_test(void);
extern void sched_init(void);
extern void schedule(void);
extern void os_main(void);

static char buffer[BUFFER_LENGTH];

void start_kernel(void)
{
	uart_init();
	uart_puts("Hello, RVOS!\n");
	page_init();
	sched_init();
	os_main();
	
	while (1) {
		uart_puts("OS: Activate next task\n");
		task_go();
		uart_puts("OS: Back to OS\n\n");
	} // stop here!
}

