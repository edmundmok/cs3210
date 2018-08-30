/*******************************************************************
* prod-con-threads.c
* Producer Consumer With C
* Compile: gcc -pthread -o prodcont prod-con-threads.c
* Run: ./prodcont
*******************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PRODUCERS  10
#define CONSUMERS  10
#define MAX_BUFFER_SIZE 100

int producer_buffer[MAX_BUFFER_SIZE];
int consumer_sum = 0;
int consumed = 0;
int produced = 0;

pthread_mutex_t m;
pthread_cond_t cv;

void *producer(void *threadid) {
  pthread_mutex_lock(&m);
  int num = (rand() % 10) + 1;
  producer_buffer[produced++] = num;
  pthread_mutex_unlock(&m);
//  pthread_cond_signal(&cv);   // if use signal here, then might need to manually signal each time on exit
  pthread_cond_broadcast(&cv);  // if use broadcast here, then don't have to manually signal each time on exit
}

void *consumer(void *threadid) {
  while (1) {
    pthread_mutex_lock(&m);
    while (consumed >= produced && consumed < PRODUCERS) {
      pthread_cond_wait(&cv, &m);
    }
    if (consumed >= PRODUCERS) {
      pthread_mutex_unlock(&m);
//      pthread_cond_signal(&cv); // signal might only wake 1 consumer
      pthread_exit(NULL);
    }
    consumer_sum += producer_buffer[consumed++];
    pthread_mutex_unlock(&m);
  }
}

int main(int argc, char *argv[]) {
  pthread_t producer_threads[PRODUCERS];
  pthread_t consumer_threads[CONSUMERS];
  long producer_threadid[PRODUCERS];
  long consumer_threadid[CONSUMERS];
  srand(time(NULL));

  int rc;
  long t1, t2;
  for (t1 = 0; t1 < PRODUCERS; t1++) {
    int tid = t1;
    producer_threadid[tid] = tid;
    printf("creating producer %d\n", tid);
    rc = pthread_create(&producer_threads[tid], NULL, producer,
                        (void *) producer_threadid[tid]);
    if (rc) {
      printf("Error: Return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  for (t2 = 0; t2 < CONSUMERS; t2++) {
    int tid = t2;
    consumer_threadid[tid] = tid;
    printf("creating consumer %d\n", tid);
    rc = pthread_create(&consumer_threads[tid], NULL, consumer,
                        (void *) consumer_threadid[tid]);
    if (rc) {
      printf("Error: Return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }

  int i;
  for (i=0; i<PRODUCERS; i++) pthread_join(producer_threads[i], NULL);
  for (i=0; i<CONSUMERS; i++) pthread_join(consumer_threads[i], NULL);

  printf("### consumer_sum final value = %d ###\n", consumer_sum);

  int sum=0;
  for (i=0; i<PRODUCERS; i++) sum += producer_buffer[i];

  printf("### correct consumer_sum final value = %d ###\n", sum);

  pthread_mutex_destroy(&m);
  pthread_cond_destroy(&cv);

  pthread_exit(NULL);

}
