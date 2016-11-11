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

void take_forks(int i)
{
    sem_wait(&mutex);
    state[i] = HUNGRY;
    test(i);
    sem_post(&mutex);
    sem_wait(&s[i]);
    printf("professor %d takes forks\n", i);
}

void put_forks(int i)
{
    sem_wait(&mutex);
    state[i] = THINKING;
    test(LEFT);
    test(RIGHT);
    sem_post(&mutex);
    printf("professor %d puts forks\n", i);
}

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

    sem_init(&mutex, 0, 1);
    for (i = 0; i < NUM_PHILO; i++) {
        sem_init(&s[i], 0, 1);
    }
    
    for (i = 0; i < NUM_PHILO; i++) {
        ids[i] = i;
        int thread_id = pthread_create(&pthread[i], NULL, &philosopher, (void *)&(ids[i]));
        if (thread_id < 0) {
            printf("ERROR: cannot creat thrad\n");
            exit(0);
        }
    }
    for (i = 0; i < NUM_PHILO; i++) {
        pthread_join(pthread[i], NULL);
    }    
    return 0;
}
