#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_MONKEYS 30

enum ball_color { RED, GREEN, BLUE, YELLOW, NONE };
typedef struct _training_monkey_t{
   enum ball_color first_ball; 
   enum ball_color second_ball;
   int index;
} training_monkey_t;

void init_training_monkey(training_monkey_t*, int);
void enter_room(training_monkey_t *);
void think(training_monkey_t *);
void* monkeys(void *);
int take_ball(void);
void release_ball(training_monkey_t*);
void leave_room(training_monkey_t*);
void print_monkey(training_monkey_t*);   

sem_t sem_authority;
sem_t sem_room;
sem_t sem_feed;
sem_t sem_print;
sem_t s[4];
int num_balls = 4;
int num_waiting_monkey = 0;
const char *ball_color_name[5] = {"RED", "GREEN", "BLUE", "YELLOW", "NONE"};

void init_training_monkey(training_monkey_t *monkey, int i)
{
    monkey->first_ball = NONE; 
    monkey->second_ball = NONE;
    monkey->index = i;
}

void print_monkey(training_monkey_t* monkey)
{
    printf("Monkey %d (%s, %s): ", monkey->index, ball_color_name[monkey->first_ball], ball_color_name[monkey->second_ball]);    
}

void think(training_monkey_t *monkey)
{
    print_monkey(monkey);
    printf("thinking\n");
    sleep(1);
}

int take_ball()
{
    int wait_result = -1;
    int ball_idx;

    while (wait_result == -1) {
        ball_idx = rand() % 3;
        wait_result = sem_trywait(&s[ball_idx]);
    }
    return ball_idx;
}

void release_ball(training_monkey_t *monkey)
{
    sem_post(&s[monkey->first_ball]);
    sem_post(&s[monkey->second_ball]);
    print_monkey(monkey);
    printf("release_balls\n");
}

void enter_room(training_monkey_t *monkey)
{
    sem_wait(&sem_room);
    print_monkey(monkey);
    printf("enter the room\n");
}

void leave_room(training_monkey_t *monkey)
{
    int wait_result;

    print_monkey(monkey);
    printf("leave the room\n");
    sem_post(&sem_room);

    wait_result = sem_trywait(&sem_feed);
    if(wait_result == -1)
    {
        print_monkey(monkey);
        printf("eat apples and finish its training\n");
    }
    else
    {
        num_waiting_monkey++;
        printf("trainer: puts bananas\n");
        print_monkey(monkey);
        printf("eats bananas\n");
        num_waiting_monkey--;

        if(num_waiting_monkey == 0)
            printf("trainer: go to sleep\n");

        sem_post(&sem_feed);
    }
}

void* monkeys(void *arg)
{
    training_monkey_t *monkey = (training_monkey_t *)arg;

    enter_room(monkey);
    sem_wait(&sem_authority);
    monkey->first_ball = take_ball();
    print_monkey(monkey);
    printf("takes %s first  ball\n", ball_color_name[monkey->first_ball]);
    think(monkey);
    monkey->second_ball = take_ball();
    print_monkey(monkey);
    printf("takes %s second ball\n", ball_color_name[monkey->second_ball]);
    think(monkey);
    release_ball(monkey);
    sem_post(&sem_authority);
    leave_room(monkey);
}

int main(int argc, const char *argv[])
{
    pthread_t pthread[NUM_MONKEYS];
    training_monkey_t monkey_arr[NUM_MONKEYS];
    int i;

    srand(time(NULL));
    sem_init(&sem_authority, 0, 2);
    sem_init(&sem_room, 0, 4);
    sem_init(&sem_feed, 0, 2);

    for (i = 0; i < 4; i++) {
        sem_init(&s[i], 0, 1);
    }
    
    for (i = 0; i < NUM_MONKEYS; i++) {
        init_training_monkey(&monkey_arr[i], i); 
        int thread_id = pthread_create(&pthread[i], NULL, &monkeys, (void *)&(monkey_arr[i]));
        if (thread_id < 0) {
            printf("ERROR: cannot creat thrad\n");
            exit(0);
        }
    }
    for (i = 0; i < NUM_MONKEYS; i++) {
        pthread_join(pthread[i], NULL);
    }    
    return 0;
}
