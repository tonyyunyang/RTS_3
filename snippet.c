#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS 3
#define PERIOD 5

// Function prototype for the task function
void* task_function(void* arg);

int main() {
  pthread_t threads[NUM_THREADS];
  int policy;
  struct sched_param param;

  // Create the threads
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, task_function, (void*)i);
  }

  // Set the scheduling policy and priority for each thread
  for (int i = 0; i < NUM_THREADS; i++) {
    policy = (i % 3 == 0) ? SCHED_FIFO : (i % 3 == 1) ? SCHED_RR : SCHED_OTHER;
    param.sched_priority = (i % 3 == 0) ? 50 : (i % 3 == 1) ? 30 : 10;
    pthread_setschedparam(threads[i], policy, &param);
  }

  // Wait for the threads to finish
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  return 0;
}

// Task function for the threads
void* task_function(void* arg) {
  int thread_num = (int)arg;
  struct timespec period = {0, PERIOD * 1000000}; // period in nanoseconds

  while (1) {
    // Perform the task
    printf("Thread %d performing task\n", thread_num);

    // Sleep for the specified period
    nanosleep(&period, NULL);
  }

  return NULL;
}
