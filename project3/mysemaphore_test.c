#include <linux/unistd.h>
#include <stdio.h>
#include <semaphore.h>
#define __NR_mysema_init 323
#define __NR_mysema_down 324
//#define __NR_mysema_down_userprio 325
#define __NR_mysema_up 326
#define __NR_mysema_release 327
#define NUM_THREAD 10

int mysema_init(int, int, int);
int mysema_down(int);
int mysema_down_userprio(int, int);
int mysema_up(int);
int mysema_release(int);
void* sema_fifotest(void*);
void* sema_userpriotest(void*);
void* sema_priotest(void*);

int mysema_init(int sema_id, int start_value, int mode)
{
    return syscall(__NR_mysema_init, sema_id, start_value, mode);
}

int mysema_down(int sema_id)
{
    return syscall(__NR_mysema_down, sema_id);
}

//int mysema_down_userprio(int sema_id, int userprio)
//{
//    return syscall(__NR_mysema_down_userprio, sema_id, userprio);

int  mysema_up(int sema_id)
{
    return syscall(__NR_mysema_up, sema_id);
}

int  mysema_release(int sema_id)
{
    return syscall(__NR_mysema_release, sema_id);
}

void *sema_fifotest(void *args)
{
    int index = *(int *)(args);
    printf("thread %d go to sleep\n", index);
    syscall(__NR_mysema_down, 0);
    printf("thread %d wake up\n", index);
}    

//void *sema_userpriotest(void *args)
//{
//    int index = *(int *)(args);
//    printf("thread %d go to sleep\n", index);
//    mysema_down_userprio(2, index);
//    printf("thread %d user prio %d wake up\n", index, index);
//}      

void *sema_priotest(void *args)
{
    int index = *(int *)(args);
    printf("thread %d go to sleep\n", index);
    mysema_down(1);
    printf("thread %d prio %d wake up\n", index, getpriority(PRIO_PROCESS, 0));
}

int main(int argc, const char *argv[])
{
    pthread_t pthread[NUM_THREAD];
    int i;
    int ids[NUM_THREAD];

    for (i = 0; i < 3; i++) {
        mysema_init(i, 0, i);
    }

    printf("============TEST: FIFO MODE===========\n");
    for (i = 0; i < 1; i++) {
        ids[i] = i;
        int thread_id = pthread_create(&pthread[i], NULL, &sema_fifotest, (void *)&(ids[i]));
        if(thread_id < 0)
        {
            printf("ERROR: cannot create thread\n");
            exit(0);
        }
        sleep(5);
        printf("thread created\n");
    }
    sleep(10);
    
    for (i = 0; i < 1; i++) {
        printf("semaup called\n");
        mysema_up(0);
    }
/*
    for (i = 0; i < NUM_THREAD; i++) {
        pthread_join(pthread[i], NULL);
    }
*/
/*
    printf("======================================\n"); 
    printf("============TEST: USERPRIO MODE===========\n"); for (i = 0; i < NUM_THREAD; i++) {
        ids[i] = i;
        int thread_id = pthread_create(&pthread[i], NULL, &sema_userpriotest, (void *)&(ids[i]));
        if(thread_id < 0)
        {
            printf("ERROR: cannot create thread\n");
            exit(0);
        }
    }

    sleep(10);
    for (i = 0; i < NUM_THREAD ; i++) {
        mysema_up(2);
    }
    for (i = 0; i < NUM_THREAD; i++) {
        pthread_join(pthread[i], NULL);
    }
    printf("======================================\n");
printf("============TEST:PRIO MODE===========\n");
    for (i = 0; i < NUM_THREAD; i++) {
        ids[i] = i;
        int thread_id = pthread_create(&pthread[i], NULL, &sema_priotest, (void *)&(ids[i]));
        if(thread_id < 0)
        {
            printf("ERROR: cannot create thread\n");
            exit(0);
        }
    }
    
    sleep(10);
    for (i = 0; i < NUM_THREAD; i++) {
        mysema_up(1);
    }
    for (i = 0; i < NUM_THREAD; i++) {
        pthread_join(pthread[i], NULL);
    }
    printf("======================================\n");
*/
    for (i = 0; i < 3; i++) {
        mysema_release(i);
    }
    return 0;
}
