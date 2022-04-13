#include "os.h"

void initlock(struct spinlock *lk) {
	lk->locked = 0; 
}

void spin_lock(struct spinlock *lk)
{
	/* test and set */
	while (__sync_lock_test_and_set(&lk->locked, 1) != 0);
}

void spin_unlock(struct spinlock *lk)
{
	lk->locked = 0;
}
