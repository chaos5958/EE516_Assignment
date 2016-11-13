#include <linux/list.h>
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/ftrace.h>

#define NUM_SEMA 10
enum sema_mode { FIFO = 0, OS_PRI = 1, USER_PRI = 2,};
typedef struct mysemaphore {
    raw_spinlock_t      lock;
    unsigned int        count;
    struct list_head    wait_list;
    enum sema_mode      mode;
    bool                is_active;
} mysemaphore_t;

typedef struct mysemaphore_waiter {
	struct list_head list;
	struct task_struct *task;
    int userprio;
	bool up;
} mysemaphore_waiter_t ;

asmlinkage long sys_mysema_init(int sema_id, int start_value, int mode);
asmlinkage long sys_mysema_down(int sema_id);
asmlinkage long sys_mysema_down_userprio(int sema_id, int priority);
asmlinkage long sys_mysema_up(int sema_id);
asmlinkage long sys_mysema_release(int sema_id);

mysemaphore_t mysema_arr[NUM_SEMA];

asmlinkage long sys_mysema_init(int sema_id, int start_value, int mode)
{
    static struct lock_class_key __key;

    if(mysema_arr[sema_id].is_active== true)
        return -1;

    if(sema_id < 0 || sema_id > 9)
        return -1;

    if(start_value < 0)
        return -1;

	mysema_arr[sema_id].lock = __RAW_SPIN_LOCK_UNLOCKED(mysema_arr[sema_id].lock);
	mysema_arr[sema_id].count = start_value;
    mysema_arr[sema_id].wait_list = (struct list_head) LIST_HEAD_INIT(mysema_arr[sema_id].wait_list);	
    mysema_arr[sema_id].mode = mode;
    mysema_arr[sema_id].is_active = 1;
    lockdep_init_map(&mysema_arr[sema_id].lock.dep_map, "semaphore.lock", &__key, 0);

    printk("sema %d is initialized\n", sema_id);
    return 0;
}

asmlinkage long sys_mysema_down(int sema_id)
{
	unsigned long flags;
    long timeout = MAX_SCHEDULE_TIMEOUT;
    long return_status = 0;

    if(sema_id < 0 || sema_id > 10)
        return -1;

    if(mysema_arr[sema_id].is_active == 0)
        return -1;

    if(mysema_arr[sema_id].mode == USER_PRI)
        return sys_mysema_down_userprio(sema_id, 100);
    
	raw_spin_lock_irqsave(&mysema_arr[sema_id].lock, flags);
	if (likely(mysema_arr[sema_id].count > 0))
		mysema_arr[sema_id].count--;
	else
    {
        struct task_struct *task = current;
        mysemaphore_waiter_t waiter;

        list_add_tail(&waiter.list, &mysema_arr[sema_id].wait_list);
        waiter.task = task;
        waiter.up = false;

        for (;;) {
            if (signal_pending_state(TASK_UNINTERRUPTIBLE, task))
                goto interrupted;
            if (unlikely(timeout <= 0))
                goto timed_out;
            __set_task_state(task, TASK_UNINTERRUPTIBLE);
            raw_spin_unlock_irq(&mysema_arr[sema_id].lock);
            timeout = schedule_timeout(timeout);
            raw_spin_lock_irq(&mysema_arr[sema_id].lock);
            if (waiter.up)
                return_status = 0;
        }

timed_out:
        list_del(&waiter.list);
        return_status = -ETIME;

interrupted:
        list_del(&waiter.list);
        return_status = -EINTR;
    }
	raw_spin_unlock_irqrestore(&mysema_arr[sema_id].lock, flags);

    printk("sema %d down end\n", sema_id);
    return return_status;
}

asmlinkage long sys_mysema_down_userprio(int sema_id, int priority)
{
    unsigned long flags;
    long timeout = MAX_SCHEDULE_TIMEOUT;
    long return_status = 0;

    if(sema_id < 0 || sema_id > 10)
        return -1;

    if(mysema_arr[sema_id].is_active == 0)
        return -1;

    if(mysema_arr[sema_id].mode == USER_PRI)
        return sys_mysema_down_userprio(sema_id, 100);
    
	raw_spin_lock_irqsave(&mysema_arr[sema_id].lock, flags);
	if (likely(mysema_arr[sema_id].count > 0))
		mysema_arr[sema_id].count--;
	else
    {
        struct task_struct *task = current;
        mysemaphore_waiter_t waiter;

        list_add_tail(&waiter.list, &mysema_arr[sema_id].wait_list);
        waiter.task = task;
        waiter.userprio = priority;
        waiter.up = false;

        for (;;) {
            if (signal_pending_state(TASK_UNINTERRUPTIBLE, task))
                goto interrupted;
            if (unlikely(timeout <= 0))
                goto timed_out;
            __set_task_state(task, TASK_UNINTERRUPTIBLE);
            raw_spin_unlock_irq(&mysema_arr[sema_id].lock);
            timeout = schedule_timeout(timeout);
            raw_spin_lock_irq(&mysema_arr[sema_id].lock);
            if (waiter.up)
                return_status = 0;
        }

timed_out:
        list_del(&waiter.list);
        return_status = -ETIME;

interrupted:
        list_del(&waiter.list);
        return_status = -EINTR;
    }
	raw_spin_unlock_irqrestore(&mysema_arr[sema_id].lock, flags);

    printk("sema %d down end\n", sema_id);
    return return_status;
}

asmlinkage long sys_mysema_up(int sema_id)
{
	unsigned long flags;

    if(sema_id < 0 || sema_id >0)
        return -1;

    if(mysema_arr[sema_id].is_active == false)
        return -1;

	raw_spin_lock_irqsave(&mysema_arr[sema_id].lock, flags);
	if (likely(list_empty(&mysema_arr[sema_id].wait_list)))
        mysema_arr[sema_id].count++;
    else
    {
        mysemaphore_waiter_t *waiter = NULL, *iter_wait;
        int priority = 0;
        switch(mysema_arr[sema_id].mode)
        {
            case FIFO:
                waiter = list_first_entry(&mysema_arr[sema_id].wait_list,
                        mysemaphore_waiter_t, list);
                break;

            case OS_PRI:
                list_for_each_entry(iter_wait, &mysema_arr[sema_id].wait_list, list)
                {
                    if(iter_wait->task->prio > priority)
                    {
                        waiter = iter_wait;
                        priority = iter_wait->task->prio;
                    }
                }
                if(waiter == NULL)
                    waiter = list_first_entry(&mysema_arr[sema_id].wait_list,
                        mysemaphore_waiter_t, list);
               break;

            case USER_PRI:
                list_for_each_entry(iter_wait, &mysema_arr[sema_id].wait_list, list)
                {
                    if(iter_wait->userprio > priority)
                    {
                        waiter = iter_wait;
                        priority = iter_wait->userprio;
                    }
                }
                if(waiter == NULL)
                    waiter = list_first_entry(&mysema_arr[sema_id].wait_list,
                        mysemaphore_waiter_t, list);
               break;

        } 
        list_del(&waiter->list);
        waiter->up = true;
        wake_up_process(waiter->task);
    }
   raw_spin_unlock_irqrestore(&mysema_arr[sema_id].lock, flags);

    printk("sema %d up end\n", sema_id);
    return 0;
}

asmlinkage long sys_mysema_release(int sema_id)
{
    if(sema_id < 0 || sema_id > 0)
        return -1;

    if(mysema_arr[sema_id].is_active == false)
        return -1;

	if (!likely(list_empty(&mysema_arr[sema_id].wait_list)))
        return -1;

    mysema_arr[sema_id].is_active = false;

    return 0;
}

