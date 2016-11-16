#include <linux/unistd.h>
#include <linux/errno.h>    
#include <linux/kernel.h>
#include <linux/sched.h>

asmlinkage long sys_mysyscall(int a, int b)
{
    printk("Student ID: 20120597\n");
    printk("mysyscall: a=%d, b=%d\n", a, b);
    return a + b;
}
