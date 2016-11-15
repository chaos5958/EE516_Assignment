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

/*************************************************************
 * FUNCTION NAME: init_training_monkey                                         
 * PARAMETER: 1) monkey: target monkey which will be initialized 2) i: monkey index                                              
 * PURPOSE: initializaing training_monkey_t structure
 ************************************************************/
void init_training_monkey(training_monkey_t *monkey, int i)
{
    monkey->first_ball = NONE; 
    monkey->second_ball = NONE;
    monkey->index = i;
}

/*************************************************************
 * FUNCTION NAME: print_monkey                                         
 * PARAMETER: 1) monkey: target monkey which will be printed all of its information                                              
 * PURPOSE: printa all information about the monkey
 ************************************************************/
void print_monkey(training_monkey_t* monkey)
{
    printf("Monkey %d (%s, %s): ", monkey->index, ball_color_name[monkey->first_ball], ball_color_name[monkey->second_ball]);    
}

/*************************************************************
 * FUNCTION NAME: think                                         
 * PARAMETER: 1) monkey: target monkey which will think                                               
 * PURPOSE: monkey thinks
 ************************************************************/
void think(training_monkey_t *monkey)
{
    print_monkey(monkey);
    printf("thinking\n");
    sleep(1);
}

/*************************************************************
 * FUNCTION NAME: take_ball                                         
 * PARAMETER: NONE                                              
 * PURPOSE: choose a free ball randomly and return its index
 ************************************************************/
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

/*************************************************************
 * FUNCTION NAME: release_ball                                         
 * PARAMETER: 1) monkey: target monkey which will release both of first and second balls                                              
 * PURPOSE: monkey releases first and second balls
 ************************************************************/
void release_ball(training_monkey_t *monkey)
{
    sem_post(&s[monkey->first_ball]);
    sem_post(&s[monkey->second_ball]);
    print_monkey(monkey);
    printf("release_balls\n");
}

/*************************************************************
 * FUNCTION NAME: enter_room                                         
 * PARAMETER: 1) monkey: target monkey which will enter the room                                              
 * PURPOSE: monkey enters the room
 ************************************************************/
void enter_room(training_monkey_t *monkey)
{
    sem_wait(&sem_room);
    print_monkey(monkey);
    printf("enter the room\n");
}

/*************************************************************
 * FUNCTION NAME: leave_room                                         
 * PARAMETER: 1) monkey: target monkey which will leavee the room                                              
 * PURPOSE: monkeys leaves the room and sem_post all semaphore it has.
 *          Then, it waits for trainers to eat banana. If two trainers already feeding monkeys, then eat apples. 
 ************************************************************/
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

/*************************************************************
 * FUNCTION NAME: monkeys                                         
 * PARAMETER: 1) args: one of monkeys in the project                                              
 * PURPOSE: do all monkey-related jobs in the project
 ************************************************************/
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
    
    //initialize semaphore
    sem_init(&sem_authority, 0, 2);
    sem_init(&sem_room, 0, 4);
    sem_init(&sem_feed, 0, 2);

    for (i = 0; i < 4; i++) {
        sem_init(&s[i], 0, 1);
    }
    
    //creating threads
    for (i = 0; i < NUM_MONKEYS; i++) {
        init_training_monkey(&monkey_arr[i], i); 
        int thread_id = pthread_create(&pthread[i], NULL, &monkeys, (void *)&(monkey_arr[i]));
        if (thread_id < 0) {
            printf("ERROR: cannot creat thrad\n");
            exit(0);
        }
    }
    //parent thread waiting for child threads to be ended
    for (i = 0; i < NUM_MONKEYS; i++) {
        pthread_join(pthread[i], NULL);
    }    
    return 0;
}
