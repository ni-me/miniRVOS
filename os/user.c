#include "os.h"

#define DELAY 1000
//#define USE_LOCK

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



static struct spinlock *user_lock = NULL;

void user_task0(int id)
{
	int cnt = 10;
	printf("Task %d: Created!\n", id);
	printf("Task %d: Back to OS\n", id);
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

	while (cnt --) {
#ifdef USE_LOCK
		task_delay(DELAY);
		spin_lock(user_lock);
#endif
		for (int i = 0; i < 5; i ++) {
			printf("Task %d: Running...( %d ) ---- [ %d ]\n", id, i, cnt);
			task_delay(DELAY);
		}
#ifdef USE_LOCK
		spin_unlock(user_lock);
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
		spin_lock(user_lock);
#endif
		for (int i = 0; i < 10; i ++) {
			printf("Task 1: Running...( %d ) ---- [ %d ]\n", i, cnt);
			task_delay(DELAY);
		}

#ifdef USE_LOCK
		spin_unlock(user_lock);
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
		spin_lock(user_lock);
#endif
		for (int i = 0; i < 10; i ++) {
			printf("Task 2: Running...( %d ) ---- [ %d ]\n", i, cnt);
			task_delay(DELAY);
		}

#ifdef USE_LOCK
		spin_unlock(user_lock);
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
	user_lock = (struct spinlock *) malloc(sizeof(struct spinlock));
	if (user_lock == NULL) {
		return;
	}
	initlock(user_lock);
#endif
	task_create(user_task1, cnt, 0, 3);
	task_create(user_task0, id0, 3, 1);
	task_create(user_task2, NULL, 0, 2);
	task_create(user_task3, NULL, 20, 2);
}

 