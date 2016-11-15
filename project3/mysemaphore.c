#include <linux/list.h>
#include <linux/compiler.h>
#include <linux/kernel.h>
#include <linux/export.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/ftrace.h>
#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/random.h>

#define NUM_SEMA 10
#define FIFO 0
#define OS_PRI 1
#define USER_PRI 2

typedef struct mysemaphore {
    raw_spinlock_t      lock; 
    unsigned int        count;
    struct list_head    wait_list;
    int                 mode;
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

static mysemaphore_t mysema_arr[NUM_SEMA];

asmlinkage long sys_mysema_init(int sema_id, int start_value, int mode)
{
    static struct lock_class_key __key;
    static bool is_first = true;

    if(!is_first && mysema_arr[sema_id].is_active== true)
    {
        printk("mysema_init: active sema\n");
        return -1;
    }

    if(sema_id < 0 || sema_id > 9)
    {
        printk("mysema_init: wrong sema id\n");
        return -1;
    }

    if(start_value < 0)
    {
        printk("mysema_init: wrong start value\n");
        return -1;
    }

    mysema_arr[sema_id].lock = __RAW_SPIN_LOCK_UNLOCKED(mysema_arr[sema_id].lock);
    mysema_arr[sema_id].count = start_value;
    //mysema_arr[sema_id].wait_list = (struct list_head) LIST_HEAD_INIT(mysema_arr[sema_id].wait_list);	
    INIT_LIST_HEAD(&mysema_arr[sema_id].wait_list);
    mysema_arr[sema_id].mode = mode;
    mysema_arr[sema_id].is_active = true;
    lockdep_init_map(&mysema_arr[sema_id]->lock.dep_map, "semaphore->lock", &__key, 0);

    printk("sema %d is initialized\n", sema_id);
    is_first = false;
    return 0;
}

asmlinkage long sys_mysema_down(int sema_id)
{
    unsigned long flags;
    long return_status = 0;

    if(sema_id < 0 || sema_id > 9)
    {
        printk("mysema_down: wrong sema_id\n");
        return -1;
    }

    if(mysema_arr[sema_id].is_active == false)
    {
        printk("mysema_down: inactive sema\n");
        return -1;
    }

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

                
        for(;;)
        {
            __set_task_state(task, TASK_UNINTERRUPTIBLE);
            raw_spin_unlock_irq(&mysema_arr[sema_id].lock);
            raw_spin_lock_irq(&mysema_arr[sema_id].lock);
            if(waiter.up == true)
            {
                printk("mysema_down: thread wake up\n");
                return_status = 0;
                break;
            }
        }
    }
    raw_spin_unlock_irqrestore(&mysema_arr[sema_id].lock, flags);
    printk("sema %d down end\n", sema_id);
    return return_status;
}

asmlinkage long sys_mysema_down_userprio(int sema_id, int priority)
{
    unsigned long flags;
    long return_status = 0;

    if(sema_id < 0 || sema_id > 9)
    {
        printk("sema_down_userprio: wrong sema_id\n");
        return -1;
    }

    if(mysema_arr[sema_id].is_active == false)
    {
        printk("sema_down_userprio: inactive sema\n");
        return -1;
    }

    raw_spin_lock_irqsave(&mysema_arr[sema_id].lock, flags);
    if (likely(mysema_arr[sema_id].count > 0))
        mysema_arr[sema_id].count--;
    else
    {
        struct task_struct *task = current;
        if(current == NULL)
            printk("mysema_down: current is NULL\n");
        mysemaphore_waiter_t waiter;

        list_add_tail(&waiter.list, &mysema_arr[sema_id].wait_list);
        waiter.task = task;
        waiter.userprio = priority;
        waiter.up = false;
        printk("mysema_down: add waiter end\n"); 

        /*
           for (;;) {
           __set_task_state(task, TASK_UNINTERRUPTIBLE);
           raw_spin_unlock_irq(&mysema_arr[sema_id].lock);
           raw_spin_lock_irq(&mysema_arr[sema_id].lock);
           if (waiter.up)
           {
           return_status = 0;
           break;
           }
           }
           */
    }
    raw_spin_unlock_irqrestore(&mysema_arr[sema_id].lock, flags);

    printk("sema %d down end\n", sema_id);
    return return_status;
}

asmlinkage long sys_mysema_up(int sema_id)
{
    unsigned long flags;

    if(sema_id < 0 || sema_id > 9)
    {
        printk("sema_up: wrong sema_id\n");
        return -1;
    }

    if(mysema_arr[sema_id].is_active == false)
    {
        printk("sema_up: inactive sema\n");
        return -1;
    }

    raw_spin_lock_irqsave(&mysema_arr[sema_id].lock, flags);
    if (likely(list_empty(&mysema_arr[sema_id].wait_list)))
        mysema_arr[sema_id].count++;
    else
    {
        mysemaphore_waiter_t *waiter = NULL, *iter_wait;
        int priority = 0;
        printk("mysema_up: mode: %d\n", mysema_arr[sema_id].mode);
        switch(mysema_arr[sema_id].mode)
        {
            case FIFO:
                waiter = list_first_entry(&mysema_arr[sema_id].wait_list,
                        mysemaphore_waiter_t, list);
                printk("FIFO mode\n");
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
                printk("OS_PRI\n");
                break;

            case USER_PRI:
                list_for_each_entry(iter_wait, &mysema_arr[sema_id].wait_list, list)
                { if(iter_wait->userprio > priority)
                    {
                        waiter = iter_wait;
                        priority = iter_wait->userprio;
                    }
                }
                if(waiter == NULL)
                    waiter = list_first_entry(&mysema_arr[sema_id].wait_list,
                            mysemaphore_waiter_t, list);
                printk("USER_PRI\n");
                break;

        } 
        if(waiter == NULL)
            printk("waiter is NULL\n");
        else
            if(waiter->task == NULL)
                printk("task is NULL\n");
        
        list_del(&waiter->list);
        printk("mysema_up: list_del\n");
        waiter->up = true;
        printk("mysema_up: waiter->up\n");
        wake_up_process(waiter->task);
        printk("mysema up: wake_up_process\n");
    }
    raw_spin_unlock_irqrestore(&mysema_arr[sema_id].lock, flags);

    printk("sema %d up end\n", sema_id);
    return 0;
}

asmlinkage long sys_mysema_release(int sema_id)
{
    static bool is_first = true;

    if(sema_id < 0 || sema_id > 9)
    {
        printk("sema_release: wrong sema_id\n");
        return -1;
    }

    if(mysema_arr[sema_id].is_active == false)
    {
        printk("sema_release: inactive sema\n");
        return -1;
    }

    if (!is_first && !likely(list_empty(&mysema_arr[sema_id].wait_list)))
    {
        printk("sema_release: remaining jobs\n");
        return -1;
    }

    mysema_arr[sema_id].is_active = false;
    is_first = false;

    return 0;
}

