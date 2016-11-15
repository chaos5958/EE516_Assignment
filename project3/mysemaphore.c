#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/random.h>

#define NUM_SEMA 10
#define FIFO 0
#define OS_PRI 1
#define USER_PRI 2

//revised version of original semaphore - mode is added
struct mysemaphore {
    raw_spinlock_t      lock; 
    unsigned int        count; 
    struct list_head    wait_list;
    int                 mode;
    bool                is_active;
};

//revised version of original semaphore - userprio is added
struct mysemaphore_waiter {
    struct list_head list;
    struct task_struct *task;
    int userprio;
    bool up;
};

asmlinkage long sys_mysema_init(int sema_id, int start_value, int mode);
asmlinkage long sys_mysema_down(int sema_id);
asmlinkage long sys_mysema_down_userprio(int sema_id, int priority);
asmlinkage long sys_mysema_up(int sema_id);
asmlinkage long sys_mysema_release(int sema_id);

static struct mysemaphore mysema_arr[10];

/*************************************************************
 * FUNCTION NAME: sys_mysema_init                                         
 * PARAMETER: 1) sema_id: index of the semaphore 2) start_value: semaphore initial count 3) mode: semaphore mode                                              
 * PURPOSE: initialize the semaphore, and make it active 
 ************************************************************/
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
    mysema_arr[sema_id].wait_list = (struct list_head) LIST_HEAD_INIT(mysema_arr[sema_id].wait_list);	
    mysema_arr[sema_id].mode = mode;
    mysema_arr[sema_id].is_active = true;
    lockdep_init_map(&mysema_arr[sema_id].lock.dep_map, "semaphore->lock", &__key, 0);
    printk("sema %d is initialized\n", sema_id);
    is_first = false;
    return 0;
}

/*************************************************************
 * FUNCTION NAME: sys_mysema_down                                         
 * PARAMETER: 1) sema_id: index of the semaphore                                              
 * PURPOSE: decrease the count, if is 0, do busy waiting 
 ************************************************************/
asmlinkage long sys_mysema_down(int sema_id)
{
    unsigned long flags;
    long return_status = 0;

    if(sema_id < 0 || sema_id > 9)
    {
        printk("mysema_down: wrong sema_id\n");
        return -1;
    }

    if(!mysema_arr[sema_id].is_active)
    {
        printk("mysema_down: inactive sema\n");
        return -1;
    }

    //if sema mode is userprio, then call sys_myseam_down_userprio
    if(mysema_arr[sema_id].mode == 2)
        return sys_mysema_down_userprio(sema_id, 100);

    //aquire the lock
    raw_spin_lock_irqsave(&mysema_arr[sema_id].lock, flags);
    //check count is bigger then 0
    //if it is bigger then 0 just decrease count
    //,else do busy waiting 
    if (likely(mysema_arr[sema_id].count > 0))
        mysema_arr[sema_id].count--;
    else
    {
        struct task_struct *task = current;
        struct mysemaphore_waiter waiter;

        printk("mysema down: sema_id: %d\n", sema_id);

        //add this task in the semaphore wainting queue
        list_add_tail(&waiter.list, &mysema_arr[sema_id].wait_list);
        waiter.task = task;
        waiter.up = false;
                
        //busy waiting
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
    //release the lock
    raw_spin_unlock_irqrestore(&mysema_arr[sema_id].lock, flags);
    printk("sema %d down end\n", sema_id);
    return return_status;
}

/*************************************************************
 * FUNCTION NAME: sys_mysema_down_userprio                                         
 * PARAMETER: 1) sema_id: index of the semaphore 2) priority: user priority for the semaphore                                              
 * PURPOSE: same as above "sysmysema_down"
 ************************************************************/
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
        struct mysemaphore_waiter waiter;
        
        list_add_tail(&waiter.list, &mysema_arr[sema_id].wait_list);
        waiter.task = task;
        waiter.userprio = priority;
        waiter.up = false;

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
    }
    raw_spin_unlock_irqrestore(&mysema_arr[sema_id].lock, flags);

    printk("sema %d down end\n", sema_id);
    return return_status;
}

/*************************************************************
 * FUNCTION NAME: sys_mysema_up                                         
 * PARAMETER: 1) sema_id: index of the semaphore                                              
 * PURPOSE: increase the semaphore's count, if there are wating task, wake one of them
 ************************************************************/
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

    //acquire the lock
    raw_spin_lock_irqsave(&mysema_arr[sema_id].lock, flags);
    //no waiting task
    if (likely(list_empty(&mysema_arr[sema_id].wait_list)))
    {
        mysema_arr[sema_id].count++;
        printk("mysema_up: count %d\n", mysema_arr[sema_id].count);
    }
    //waiting task exists
    else
    {
        struct mysemaphore_waiter
            *waiter = NULL, *iter_wait;
        int priority = -1;
        printk("mysema_up: mode: %d\n", mysema_arr[sema_id].mode);
        switch(mysema_arr[sema_id].mode)
        {
            //fifo mode - select the first task in the semaphore's waiting queue
            case 0:
                waiter = list_first_entry(&mysema_arr[sema_id].wait_list,
                        struct mysemaphore_waiter
                        , list);
                printk("FIFO mode\n");
                break;

            //os_priority mode - select the task who has smallest prio
            case 1:
                list_for_each_entry(iter_wait, &mysema_arr[sema_id].wait_list, list)
                {
                    if(priority == -1 || iter_wait->task->prio < priority)
                    {
                        waiter = iter_wait;
                        priority = iter_wait->task->prio;
                    }
                }
                if(waiter == NULL)
                    waiter = list_first_entry(&mysema_arr[sema_id].wait_list,
                            struct mysemaphore_waiter
                            , list);
                printk("OS_PRI\n");
                break;

            //user_priority mode - select the task who has highest userprio
            case 2:
                list_for_each_entry(iter_wait, &mysema_arr[sema_id].wait_list, list)
                { if(iter_wait->userprio > priority)
                    {
                        waiter = iter_wait;
                        priority = iter_wait->userprio;
                    }
                }
                if(waiter == NULL)
                    waiter = list_first_entry(&mysema_arr[sema_id].wait_list,
                            struct mysemaphore_waiter
                            , list);
                printk("USER_PRI\n");
                break;

        } 
        if(waiter == NULL)
            printk("waiter is NULL\n");
        else
            if(waiter->task == NULL)
                printk("task is NULL\n");
       
        //wake up the target semaphore
        list_del(&waiter->list);
        printk("mysema_up: list_del\n");
        waiter->up = true;
        printk("mysema_up: waiter->up\n");
        wake_up_process(waiter->task);
        printk("mysema up: wake_up_process\n");
    }
    //release the lock
    raw_spin_unlock_irqrestore(&mysema_arr[sema_id].lock, flags);

    printk("sema %d up end\n", sema_id);
    return 0;
}

/*************************************************************
 * FUNCTION NAME: sys_mysema_release                                         
 * PARAMETER: 1) sema_id: index of the semaphore                                              
 * PURPOSE: make the semaphore inactive
 ************************************************************/
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

