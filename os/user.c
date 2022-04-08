#include "os.h"

#define DELAY 1000

void user_task0(int id)
{
	int n = 10;
	printf("Task %d: Created!\n", id);
	printf("Task %d: Back to OS\n", id);
	task_yeild();
	while (n--) {
		printf("Task %d: Running...( %d )\n", id, n);
		task_delay(DELAY);
		task_yeild();
	}
	printf("Task %d: Exited!\n", id);
	task_exit();
}


void user_task1(int cnt)
{
	uart_puts("Task 1: Created!\n");
	uart_puts("Task 1: Back to OS\n");
	task_yeild();

	while (1) {
		printf("Task 1: Running...( %d )\n", cnt);
		task_delay(DELAY);
		//task_yeild();
	}
	uart_puts("Task 1: Exited!\n");
	task_exit();
}


void user_task2()
{
	int cnt = 2;
	uart_puts("Task 2: Created!\n");
	uart_puts("Task 2: Back to OS\n");
	task_yeild();
	while (1) {
		printf("Task 2: Running...( %d )\n", cnt);
		task_delay(DELAY);
		//task_yeild();
	}
	uart_puts("Task 2: Exited!\n");
	 task_exit();
}

void user_task3()
{
	int n = 15;
	uart_puts("Task 3: Created!\n");
	uart_puts("Task 3: Back to OS\n");
	task_yeild();
	while (n--) {
		printf("Task 3: Running... ( %d )\n", n);
		task_delay(DELAY);
		task_yeild();
	}

	uart_puts("Task 3: Exited!\n");
	task_exit();
}


/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	int id0 = 0;
	int cnt = 1;

	// task_create(user_task0, id0, 20, 5);
	// task_create(user_task3, NULL, 255, 6);
	
	task_create(user_task1, cnt, 0);
	//task_create(user_task0, id0, 3);
	task_create(user_task2, NULL, 0);
	//task_create(user_task3, NULL, 20);
}

 