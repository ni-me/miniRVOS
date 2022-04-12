#include "os.h"

/*
#define USE_LOCK

static struct spinlock *user_lock = NULL;
*/

struct userdata {
	int counter;
	char *str;
};

/* Jack must be global */
struct userdata person = {0, "Jack"};

void timer_func(void *arg)
{
	if (NULL == arg)
		return;

	struct userdata *param = (struct userdata *)arg;

	param->counter++;
	printf("======> TIMEOUT: %s: %d\n", param->str, param->counter);
}



void user_task0(void *arg)
{
	int id = *(int *)arg;
	int cnt = 10;
	printf("Task %d: Created!\n", id);
	printf("Task %d: Back to OS\n", id);
	task_yeild();

	while (cnt --) {
#ifdef USE_LOCK
		wait(DELAY);
		spin_lock(user_lock);
#endif
		for (int i = 0; i < 5; i ++) {
			printf("Task %d: Running...( %d ) ---- [ %d ]\n", id, i, cnt);
			wait(DELAY);
		}
#ifdef USE_LOCK
		spin_unlock(user_lock);
		wait(DELAY);
#endif
	}
	printf("Task %d: Exited!\n", id);
	task_exit();
}


void user_task1(void *arg)
{
	uart_puts("Task 1: Created!\n");
	uart_puts("Task 1: Back to OS\n");
	task_yeild();

	struct timer *t1 = timer_create(timer_func, &person, 3);
	if (t1 == NULL) {
		printf("timer_create() failed!\n");
	}
	struct timer *t2 = timer_create(timer_func, &person, 5);
	if (t2 == NULL) {
		printf("timer_create() failed!\n");
	}
	struct timer *t3 = timer_create(timer_func, &person, 7);
	if (t3 == NULL) {
		printf("timer_create() failed!\n");
	}

	int cnt = *(int *)arg;
	while (cnt --) {
#ifdef USE_LOCK
		wait(DELAY);
		spin_lock(user_lock);
#endif
		for (int i = 0; i < 10; i ++) {
			printf("Task 1: Running...( %d ) ---- [ %d ]\n", i, cnt);
			wait(DELAY);
		}

#ifdef USE_LOCK
		spin_unlock(user_lock);
		wait(DELAY);
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
		wait(DELAY);
		spin_lock(user_lock);
#endif
		for (int i = 0; i < 10; i ++) {
			printf("Task 2: Running...( %d ) ---- [ %d ]\n", i, cnt);
			wait(DELAY);
		}

#ifdef USE_LOCK
		spin_unlock(user_lock);
		wait(DELAY);
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
		wait(DELAY);
	}
}


void user_task4()
{
	uart_puts("Task 4: Created!\n");
	uart_puts("Task 4: Back to OS\n");
	task_yeild();

	wait(DELAY);
	uart_puts("Task 4: Delay...\n");    
	task_delay(2);
	uart_puts("Task 4: I'm back!\n");

	while (1) {
		uart_puts("Task 4: I'm runing...\n");
		wait(DELAY);
	}
}



void user_task5()
{
	uart_puts("Task 5: Created!\n");
	uart_puts("Task 5: Back to OS\n");
	task_yeild();

	wait(DELAY);
	uart_puts("Task 5: Delay...\n");
	task_delay(3);
	uart_puts("Task 5: I'm back!\n");

	while (1) {
		uart_puts("Task 5: I'm runing...\n");
		wait(DELAY);
	}
}


/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	int id0 = 0;
	int cnt = 3;
#ifdef USE_LOCK
	user_lock = (struct spinlock *) malloc(sizeof(struct spinlock));
	if (user_lock == NULL) {
		return;
	}
	initlock(user_lock);
#endif
	task_create(user_task1, &cnt, 0, 1);
	task_create(user_task0, &id0, 3, 1);
	task_create(user_task2, NULL, 0, 2);
	task_create(user_task3, NULL, 20, 2);

	task_create(user_task4, NULL, 0, 2);
	task_create(user_task5, NULL, 0, 1);
}

 