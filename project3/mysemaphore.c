#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/random.h>

#define NUM_SEMA 10

struct mysemaphore {
	raw_spinlock_t lock; // lock 
	unsigned int count; // semaphore value
	struct list_head wait_list; // semaphore_waiters structures for this semaphore
	int mode; // 0 for FIFO non-0 for alternative(random)
	bool isActive; // 0 for inactive, 1 for active
};

struct semaphore_waiter {
	struct list_head list; // list structrue
	struct task_struct *task; // waiting process or threads
	bool up; // waiter's status: whether it is waiting for semaphore or not
};

// global 10 semaphores used in my_semaphore.
static struct mysemaphore my_sema[10];

asmlinkage long sys_mysema_init (int sema_id, int start_value, int mode){
	static struct lock_class_key __key;

	// check validity of sema_id
	if (sema_id < 0 || sema_id > 9)
		return -1;

	// check whether semaphore is already active.
	if (my_sema[sema_id].isActive)
		return -1;

	// check validity of start value.
	if (start_value < 0)
		return -1;

	// initialize the lock
	my_sema[sema_id].lock = __RAW_SPIN_LOCK_UNLOCKED(my_sema[sema_id].lock);
	// initialize the semaphore value with start_value
	my_sema[sema_id].count = start_value;
	// initialize the list for waiters 
	INIT_LIST_HEAD(&my_sema[sema_id].wait_list);
	// assigning mode
	my_sema[sema_id].mode = mode;
	// activate the semaphore
	my_sema[sema_id].isActive = true;
	// initialize the lockdep map.
	lockdep_init_map(&my_sema[sema_id]->lock.dep_map, "semaphore->lock", &__key, 0);
	return 0;
}

asmlinkage long sys_mysema_down (int sema_id){
	unsigned long flags;

	// check validity of sema_id
	if (sema_id < 0 || sema_id > 9)
		return -1;

	// check whether semaphore is active
	if (!my_sema[sema_id].isActive)
		return -1;

	// acquire the lock
	raw_spin_lock_irqsave(&my_sema[sema_id].lock,flags);
	if (likely (my_sema[sema_id].count > 0))
		// if semaphore value is more than 0
		my_sema[sema_id].count--;
	else{
		// if semaphore value is less than or equal to 0, it needs to sleep.
		struct task_struct *task = current;	// current task_struct
		struct semaphore_waiter waiter;

		// add to the semaphores waiter list.
		list_add_tail (&waiter.list, &my_sema[sema_id].wait_list);
		waiter.task = task;
		waiter.up = false;

		// busy waiting until waiter.up to be true.
		for(;;){
			__set_task_state(task, TASK_UNINTERRUPTIBLE);
			raw_spin_unlock_irq(&my_sema[sema_id].lock);
			raw_spin_lock_irq(&my_sema[sema_id].lock);
			if (waiter.up)
				break;
		}
	}
	// release the lock.
	raw_spin_unlock_irqrestore(&my_sema[sema_id].lock, flags);
	return 0;
}

asmlinkage long sys_mysema_up (int sema_id){
	unsigned long flags;
	
	// check validity of sema_id
	if (sema_id < 0 || sema_id > 9)
		return -1;

	// check whether semaphore is active or not
	if (!my_sema[sema_id].isActive)
		return -1;

	// acquire the lock.
	raw_spin_lock_irqsave(&my_sema[sema_id].lock, flags);
	if (likely(list_empty(&my_sema[sema_id].wait_list)))
		// if there is no waiter for semaphore, it just need to increment the semaphore value.
		my_sema[sema_id].count++;
	else if (my_sema[sema_id].mode == 0){ // when there is waiter for the semaphore.
		// FIFO mode
		// take first waiter of the waiter list
		struct semaphore_waiter *waiter = list_first_entry (&my_sema[sema_id].wait_list,
				struct semaphore_waiter, list);
		// remove it from the waiter list
		list_del (&waiter->list);
		// change semaphore_waiter's up variable to true so that it can break busy waiting.
		waiter->up = true;
		// wake up the process.
		wake_up_process(waiter->task);
	}else {
		// alternative mode: random mode
		struct list_head *iter_head;
		struct semaphore_waiter *waiter;
		int count = 0;
		unsigned int idx;
		int i;

		// count how many waiters exist
		list_for_each(iter_head, &my_sema[sema_id].wait_list){
			count ++;
		}

		// get random index range from 0 to number_of_waiter-1
		idx = get_random_int ();
		idx = idx%count;
		
		// get process from the waiter list with randomly generated index above.
		iter_head = (&my_sema[sema_id].wait_list)->next;
		for(i=0; i<idx; ++i)
			iter_head = iter_head->next;

		// get process to wake up
		waiter = list_entry (iter_head, struct semaphore_waiter, list);
		// remove from the waiter list
		list_del (&waiter->list);
		// change semaphore_waiter's up variable to true so that it can break busy waiting.
		waiter->up = true;
		// wake up the process.
		wake_up_process(waiter->task);	
	}
	// release the lock
	raw_spin_unlock_irqrestore(&my_sema[sema_id].lock, flags);
	return 0;
}

asmlinkage long sys_mysema_release (int sema_id){
	// check validity of the sema_id
	if (sema_id < 0 || sema_id > 9)
		return -1;

	// check whether semaphore is active or not
	if (!my_sema[sema_id].isActive)
		return -1;

	// assure there is no waiter for this semaphore.
	if (!list_empty(&my_sema[sema_id].wait_list))
		return -1;

	// deactivate the semaphore.
	my_sema[sema_id].isActive = false;
	return 0;
}
