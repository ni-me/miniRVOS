#include "os.h"

/*
 * Following functions SHOULD be called ONLY ONE time here,
 * so just declared here ONCE and NOT included in file os.h.
 */
extern void uart_init(void);
extern void page_init(void);
extern void page_test(void);

const int BUFFER_LENGTH = 100;

void start_kernel(void)
{
	uart_init();
	uart_puts("Hello, RVOS!\n");

	page_init();
	page_test();
	
	char buffer[BUFFER_LENGTH];
	while (1) {
		uart_gets(buffer);
		uart_puts("\n");
		uart_puts(buffer);
		uart_puts("\n");
	}; // stop here!
}

