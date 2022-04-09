#include "os.h"

#define DELAY 1000
#define USE_LOCK

struct spinlock *lock = NULL;


void user_task0(int id)
{
	int n = 10;
	printf("Task %d: Created!\n", id);
	printf("Task %d: Back to OS\n", id);
	task_yeild();

	while (n --) {
#ifdef USE_LOCK
		task_delay(DELAY);
		spin_lock(lock);
#endif
		for (int i = 0; i < 5; i ++) {
			printf("Task %d: Running...( %d )\n", id, i);
			task_delay(DELAY);
		}
#ifdef USE_LOCK
		spin_unlock(lock);
		task_delay(DELAY);
#endif
	}
	printf("Task %d: Exited!\n", id);
	task_exit();
}


void user_task1(int cnt)
{
	uart_puts("Task 1: Created!\n");
	uart_puts("Task 1: Back to OS\n");
	task_yeild();

	while (cnt --) {
#ifdef USE_LOCK
		task_delay(DELAY);
		spin_lock(lock);
#endif
		for (int i = 0; i < 10; i ++) {
			printf("Task 1: Running...( %d )\n", i);
			task_delay(DELAY);
		}

#ifdef USE_LOCK
		spin_unlock(lock);
		task_delay(DELAY);
#endif
	}
	uart_puts("Task 1: Exited!\n");
	task_exit();
}


void user_task2()
{
	int cnt = 5;
	uart_puts("Task 2: Created!\n");
	uart_puts("Task 2: Back to OS\n");
	task_yeild();
	while (cnt --) {
#ifdef USE_LOCK
		task_delay(DELAY);
		spin_lock(lock);
#endif
		for (int i = 0; i < 10; i ++) {
			printf("Task 2: Running...( %d )\n", i);
			task_delay(DELAY);
		}

#ifdef USE_LOCK
		spin_unlock(lock);
		task_delay(DELAY);
#endif
	}
	uart_puts("Task 2: Exited!\n");
	task_exit();
}

void user_task3()
{
	int n = -1;
	uart_puts("Task 3: Created!\n");
	uart_puts("Task 3: Back to OS\n");
	task_yeild();
	while (1) {
		n = (n + 1) % 15;
		printf("Task 3: Running... ( %d )\n", n);
		task_delay(DELAY);
	}
}


/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	int id0 = 0;
	int cnt = 3;
#ifdef USE_LOCK
	lock = (struct spinlock *) malloc(sizeof(struct spinlock));
	if (lock == NULL) {
		return;
	}
	initlock(lock);
#endif
	task_create(user_task1, cnt, 0);
	task_create(user_task0, id0, 3);
	task_create(user_task2, NULL, 0);
	task_create(user_task3, NULL, 20);
}

 