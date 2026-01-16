#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <getopt.h>
#include <sched.h>
#include <unistd.h>
#include <time.h>

typedef struct{
    int id;
    double time_wait;
    int policy;
    int priority;
    pthread_barrier_t* barrier;
}thread_info_t;

void *thread_func(void *thread_info){

    thread_info_t *info = (thread_info_t *)thread_info;

    /* Wait until all threads are ready */
    pthread_barrier_wait(info->barrier);

    /* Do the task */ 
    for(int i=0 ; i<3 ; i++){
        printf("Thread %d is running\n", info->id);

        struct timespec start, current;
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);  // the time has passed

        double spend_time = 0.0;
        while (spend_time < info->time_wait){
            
            clock_gettime(CLOCK_THREAD_CPUTIME_ID, &current);
            spend_time = (current.tv_sec - start.tv_sec) + (current.tv_nsec - start.tv_nsec)/1000000000.0;
        }
    }

    return NULL;
}

int main(int argc, char *argv[]){
    
    int num_threads = 0;
    double time_wait = 0.0;
    char *policies = NULL;
    char *priorities = NULL;
    char *p_pos, *pri_pos;

    /* Parse program arguments */
    int flag;
    while( (flag = getopt(argc, argv, "n:t:s:p:")) != -1){
        switch(flag){
            case 'n':
                num_threads = atoi(optarg);
                break;
            case 't':
                time_wait = atof(optarg);
                break;
            case 's':
                policies = strdup(optarg);
                break;
            case 'p':
                priorities = strdup(optarg);
                break;
        }
    }

    /* Set CPU affinity */
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);      // clean up CPU set
    CPU_SET(0, &cpuset);    // put CPU0 to CPU set
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset); 
    
    /* Initital thread barrier */
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, num_threads);

    thread_info_t thread_info[num_threads];
    pthread_t thread[num_threads];
    char *p_ptr = strtok_r(policies, ",", &p_pos);
    char *pri_ptr = strtok_r(priorities, ",", &pri_pos);

    /* Create <num_threads> worker threads */
    for(int i=0 ; i<num_threads ; ++i){
        
        // printf("Loop %d, p_ptr : %s, pri_ptr : %s\n", i, p_ptr ? p_ptr : "NULL", pri_ptr ? pri_ptr : "NULL");
        if(p_ptr == NULL || pri_ptr == NULL)
            break;
        
        thread_info[i].id = i;
        thread_info[i].barrier = &barrier;
        thread_info[i].time_wait = time_wait;
        thread_info[i].priority = atoi(pri_ptr);

        if(strcmp(p_ptr, "NORMAL") == 0)
            thread_info[i].policy = SCHED_OTHER;
        else
            thread_info[i].policy = SCHED_FIFO;
     
        /* setting threads' attribute */
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);    // don't inheritence scheduleing policy from main
        pthread_attr_setschedpolicy(&attr, thread_info[i].policy);      // set scheduling policy

        if(thread_info[i].policy == SCHED_FIFO){
            struct sched_param param;
            param.sched_priority = thread_info[i].priority;
            pthread_attr_setschedparam(&attr, &param);
        }

        /*int ret = pthread_create(&thread[i], &attr, thread_func ,&thread_info[i]);
        if (ret != 0) {
            perror("pthread_create failed"); 
            exit(1);
        }
        printf("Creating thread %d with policy %s\n", i, p_ptr);*/
        pthread_create(&thread[i], &attr, thread_func ,&thread_info[i]);

        p_ptr = strtok_r(NULL, ",", &p_pos);
        pri_ptr = strtok_r(NULL, ",", &pri_pos);
        pthread_attr_destroy(&attr);
    }

    /* Wait for all threads to finish  */
    for(int i=0 ; i<num_threads ; ++i)
        pthread_join(thread[i], NULL);

    pthread_barrier_destroy(&barrier);
    return 0;
}