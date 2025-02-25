#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include "a2_helper.h"

//used in task 2
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t end_condition = PTHREAD_COND_INITIALIZER;
int thread1_started_flag = 0;

//used in task 3
sem_t barrier;
sem_t print_barrier;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; 
int thread11_running_flag = 0;
int running_threads = 0;
int total_threads = 0;

//used in task 4
sem_t *start_semaphore;
sem_t *end_semaphore;  

//function performed by thread 4 of process 6
void* thread64Function() 
{
    //starts the execution
    pthread_mutex_lock(&mutex);
    info(BEGIN, 6, 4);

    //waits for thread 1 to be running and waiting
    while(thread1_started_flag != 1) {
        pthread_cond_wait(&start_condition, &mutex);
    } 
    //signals the proper start of the execution
    pthread_cond_signal(&start_condition);

    //waits for the end of thread 1's execution
    pthread_cond_wait(&end_condition, &mutex);
    info(END, 6, 4);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

//function performed by thread 1 of process 6
void* thread61Function() 
{
    //signals thread 4 that it started the execution and is ready
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&start_condition);
    thread1_started_flag = 1;

    //waits for thread 4 to properly start its execution
    pthread_cond_wait(&start_condition, &mutex);
    info(BEGIN, 6, 1);

    //finishes the execution and lets thread 4 know
    info(END, 6, 1);
    pthread_cond_signal(&end_condition);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

//function performed by thread 2 of process 6
void* thread62Function() 
{
    //waits for the termination of the thread 2.3
    sem_wait(start_semaphore); 

    info(BEGIN, 6, 2);
 
    info(END, 6, 2);

    //allows thread 2.2 to start
    sem_post(end_semaphore);

    return NULL;
}

//function performed by thread 3 of process 6
void* generic6ThreadFunction(void *arg) 
{
    long int threadNum = (long int)arg;
    int threadNumInt = (int)threadNum;
    info(BEGIN, 6, threadNumInt);
    info(END, 6, threadNumInt);

    return NULL;
}

void synchronizingThreads() 
{
    pthread_t thread1, thread2, thread3, thread4;

    //creating the threads
    pthread_create(&thread1, NULL, thread61Function, NULL);
    pthread_create(&thread2, NULL, thread62Function, NULL);
    pthread_create(&thread3, NULL, generic6ThreadFunction, (void*)3);
    pthread_create(&thread4, NULL, thread64Function, NULL);

    //waiting for the threads
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);

    return;
}

//function performed by the threads in the 4th process (but 11)
void* genericLimitedThreadFunction(void* arg) 
{
    int threadNumInt = *((int *)arg);

    //waits so that there are no more than 4 threads running
    sem_wait(&barrier);
    info(BEGIN, 4, threadNumInt);

    //increases the number of running threads and signals that is has started (for T4.11)
    pthread_mutex_lock(&mutex1);
    running_threads++;
    total_threads++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex1);

    //in case thread 11 is running, make sure it prints the end message only after it
    if(thread11_running_flag == 1) {
        sem_wait(&print_barrier);
        info(END, 4, threadNumInt);
        sem_post(&print_barrier);
    }
    else {
        info(END, 4, threadNumInt);
    }

    //decreases the number of running threads (POSSIBLE IMPROVEMENT?)
    pthread_mutex_lock(&mutex1);
    running_threads--;
    pthread_mutex_unlock(&mutex1);

    sem_post(&barrier);

    return NULL;
}

void* thread411Function(void* arg) 
{
    sem_wait(&barrier);

    thread11_running_flag = 1;
    info(BEGIN, 4, 11);

    //printf("RUNNING THREADS: %d\n", running_threads);
    //pthread_mutex_lock(&mutex1);

    //as long as there are no 3 other threads running at the same time, it waits
    while (running_threads < 3) {
        if(total_threads==46) {
            break;
        }
        pthread_mutex_lock(&mutex1);
        pthread_cond_wait(&cond, &mutex1);
        pthread_mutex_unlock(&mutex1);
    }
    //printf("RUNNING THREADS: %d\n", running_threads);
    info(END, 4, 11);
    //pthread_mutex_unlock(&mutex1);

    thread11_running_flag = 0;
    sem_post(&print_barrier);

    sem_post(&barrier);

    return NULL;
}

void processBarrier() 
{
    pthread_t threads[47];
    int thread_ids[47];

    sem_init(&barrier, 0, 4);
    sem_init(&print_barrier, 0, 0);

    thread_ids[10] = 11;
    pthread_create(&threads[10], NULL, thread411Function, NULL);

    for (int i = 0; i < 47; i++) {
        thread_ids[i] = i + 1;
        if (i == 10) {continue;}
        pthread_create(&threads[i], NULL, genericLimitedThreadFunction, (void *)&thread_ids[i]);
    }

    for (int i = 0; i < 47; i++) {
        pthread_join(threads[i], NULL);
    }
}

//function performed by all the threads in process 2 (but 3 and 6)
void* genericThreadFunction2(void *arg) 
{
    long int threadNum = (long int)arg;
    int threadNumInt = (int)threadNum;
    info(BEGIN, 2, threadNumInt);
    info(END, 2, threadNumInt);

    return NULL;
}

//function performed by thread 3 of process 6
void* thread23Function() 
{   
    info(BEGIN, 2, 3);
   
    info(END, 2, 3);

    //mark the end of the thread so T6.2 can start
    sem_post(start_semaphore);

    return NULL;
}

//function performed by thread 3 of process 6
void* thread26Function() 
{
    //wait for T6.2 to finish
    sem_wait(end_semaphore); 

    info(BEGIN, 2, 6);

    info(END, 2, 6);

    return NULL;
}

void differentProcessesThreads() 
{
    pthread_t thread1, thread2, thread3, thread4, thread5, thread6;

    //creating the threads
    pthread_create(&thread1, NULL, genericThreadFunction2, (void*)1);
    pthread_create(&thread2, NULL, genericThreadFunction2, (void*)2);
    pthread_create(&thread3, NULL, thread23Function, NULL);
    pthread_create(&thread4, NULL, genericThreadFunction2, (void*)4);
    pthread_create(&thread5, NULL, genericThreadFunction2, (void*)5);
    pthread_create(&thread6, NULL, thread26Function, NULL);

    //waiting for the threads
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    pthread_join(thread5, NULL);
    pthread_join(thread6, NULL);

    return;
}

int main()
{
    init();

    info(BEGIN, 1, 0);

    //inter-process sempahores
    start_semaphore = sem_open("/start_semaphore", O_CREAT, 0644, 0);
    if (start_semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    end_semaphore = sem_open("/end_semaphore", O_CREAT, 0644, 0);
    if (end_semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    pid_t pid_p2, pid_p3, pid_p4, pid_p5, pid_p6, pid_p7, pid_p8, pid_p9;

    pid_p2 = fork(); //creating process 2
    switch (pid_p2)
    {
    case -1:
        perror("process 2 was not created");
        exit(1);
    case 0: //we are in process 2
        info(BEGIN, 2, 0);

        pid_p3 = fork(); //creating process 3
        switch (pid_p3)
        {
        case -1:
            perror("process 3 was not created");
            exit(1);
        case 0: //we are in process 3
            info(BEGIN, 3, 0);

            pid_p5 = fork(); //creating process 5
            switch (pid_p5)
            {
            case -1:
                perror("process 5 was not created");
                exit(1);
            case 0://we are in process 5
                info(BEGIN, 5, 0);
                info(END, 5, 0);
                exit(0);
            default: //we are in process 3
                wait(NULL);
                break;
            }
            //still in process 3
            info(END, 3, 0);
            exit(0);
        default: //we are in process 2
            wait(NULL);
            break;
        }
        //still in process 2
        pid_p4 = fork(); //creating process 4
        switch (pid_p4)
        {
        case -1:
            perror("process 4 was not created");
            exit(1);
        case 0: //we are in process 4
            info(BEGIN, 4, 0);

            processBarrier();

            info(END, 4, 0);
            exit(0);
        default: //we are in process 2
            wait(NULL);
            break;
        }
        //still in process 2
        pid_p6 = fork(); //creating process 6
        switch (pid_p6)
        {
        case -1:
            perror("process 6 was not created");
            exit(1);
        case 0: //we are in process 6
            info(BEGIN, 6, 0);

            synchronizingThreads();

            info(END, 6, 0);
            exit(0);
        default: //we are in process 2
            differentProcessesThreads();
            wait(NULL);
            break;
        }
        //still in process 2
        pid_p8 = fork(); //creating process 8
        switch (pid_p8)
        {
        case -1:
            perror("process 8 was not created");
            exit(1);
        case 0: //we are in process 8
            info(BEGIN, 8, 0);
            info(END, 8, 0);
            exit(0);
        default: //we are in process 2
            wait(NULL);
            break;
        }
        //still in process 2
        pid_p9 = fork(); //creating process 9
        switch (pid_p9)
        {
        case -1:
            perror("process 9 was not created");
            exit(1);
        case 0: //we are in process 9
            info(BEGIN, 9, 0);
            info(END, 9, 0);
            exit(0);
        default: //we are in process 2
            wait(NULL);
            break;
        }
        //still in process 2
        info(END, 2, 0);
        exit(0);
    default: //we are in process 1
        pid_p7 = fork(); //creating process 7
        switch (pid_p7)
        {
        case -1:
            perror("process 7 was not created");
            exit(1);
        case 0: //we are in process 7
            info(BEGIN, 7, 0);
            info(END, 7, 0);
            exit(0);
        default: //we are in process 1
            wait(NULL);
            break;
        }
        //still in process 1
        wait(NULL);

        info(END, 1, 0);
        break;
    }

    pthread_cond_destroy(&start_condition);
    pthread_cond_destroy(&end_condition);
    pthread_mutex_destroy(&mutex);

    sem_destroy(&barrier);
    sem_destroy(&print_barrier);
    pthread_mutex_destroy(&mutex1);
    pthread_cond_destroy(&cond);

    sem_close(start_semaphore);
    sem_unlink("/start_semaphore");
    sem_close(end_semaphore);
    sem_unlink("/end_semaphore");

    return 0;
}