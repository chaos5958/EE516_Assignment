#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#define NUM_PHILO   5
#define LEFT        (i+NUM_PHILO-1)%NUM_PHILO
#define RIGHT       (i+1)%NUM_PHILO
#define THINKING    0
#define HUNGRY      1
#define EATING      2

void *philosopher(void *);
void take_forks(int);
void put_forks(int);
void think(int);
void eat(int);
void test();
int state[NUM_PHILO];
sem_t s[NUM_PHILO];
sem_t mutex;

void think(int i)
{
    printf("professor %d think\n", i);
    sleep(1);
}

void eat(int i)
{
    printf("professor %d eat\n", i);
    sleep(1);
}

void* philosopher(void *arg)
{
    int i = *(int *)(arg);
    while (true) {
        think(i);
        take_forks(i);
        eat(i);
        put_forks(i);
    }
}

/*************************************************************
 * FUNCTION NAME: take_forks                                         
 * PARAMETER: 1) int i: philosopher index                                              
 * PURPOSE: take both of left and rigt forks simultaneously, if one of them are already eating, philospher goes to sleep
 ************************************************************/
void take_forks(int i)
{
    sem_wait(&mutex);
    state[i] = HUNGRY;
    test(i);
    sem_post(&mutex);
    sem_wait(&s[i]);
    printf("professor %d takes forks\n", i);
}

/*************************************************************
 * FUNCTION NAME: put_forks                                         
 * PARAMETER: 1) int i: philosopher index                                              
 * PURPOSE: put bot of left and right forks, then, test both of left and right philosopher if they are possible to take two forks. If the philosopher can do it, wake that sleeping philosopher 
 ************************************************************/
void put_forks(int i)
{
    sem_wait(&mutex);
    state[i] = THINKING;
    test(LEFT);
    test(RIGHT);
    sem_post(&mutex);
    printf("professor %d puts forks\n", i);
}

/*************************************************************
 * FUNCTION NAME: test                                         
 * PARAMETER: 1) int i: index of the philosopher                                              
 * PURPOSE: test whether the philosopher can take both forks. If is possible sema_pose, else do nothing
 ************************************************************/
void test(int i)
{
    if (state[i] == HUNGRY && state[LEFT] != EATING && state[RIGHT] != EATING) {
        state[i] = EATING;
        sem_post(&s[i]);      
    }
}
int main(int argc, const char *argv[])
{
    pthread_t pthread[NUM_PHILO];
    int i;
    int ids[NUM_PHILO];

    //initializing semaphores
    sem_init(&mutex, 0, 1);
    for (i = 0; i < NUM_PHILO; i++) {
        sem_init(&s[i], 0, 1);
    }
    
    //create threads which runs philosopher()
    for (i = 0; i < NUM_PHILO; i++) {
        ids[i] = i;
        int thread_id = pthread_create(&pthread[i], NULL, &philosopher, (void *)&(ids[i]));
        if (thread_id < 0) {
            printf("ERROR: cannot creat thrad\n");
            exit(0);
        }
    }
    //parent thread wait for child to be terminated
    for (i = 0; i < NUM_PHILO; i++) {
        pthread_join(pthread[i], NULL);
    }    
    return 0;
}
