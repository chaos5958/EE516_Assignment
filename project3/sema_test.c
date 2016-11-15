#include <semaphore.h>
#include <stdio.h>
#include <linux/unistd.h>

#define NUM_THREAD 3

#define _MY_SEMA_INIT 323
#define _MY_SEMA_DOWN 324
#define _MY_SEMA_UP 326 
#define _MY_SEMA_RELEASE 327

void *pthread_func (void *arg){
    int id = *((int *)arg);
    syscall(_MY_SEMA_DOWN, 0);
    printf("Thread id : %d\n",id);
}

int main(){
    int i;
    int ids[NUM_THREAD];
    pthread_t pthread[NUM_THREAD];

    for(i=0;i<NUM_THREAD;++i){
        ids[i] = i;
        pthread_create (&pthread[i], NULL, pthread_func, (void *)&(ids[i]));
        sleep (10);
    }

    for(i=0; i<NUM_THREAD; ++i){
        usleep (10);
        syscall(_MY_SEMA_UP,0);
    }

    for(i=0; i<NUM_THREAD; ++i)
        pthread_join (pthread[i], NULL);

    syscall(_MY_SEMA_RELEASE, 0);

    return 0;
}

