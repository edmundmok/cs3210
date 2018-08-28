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

#define PRODUCERS  2
#define CONSUMERS  1
#define MAX_BUFFER_SIZE 10

int producer_buffer[MAX_BUFFER_SIZE];
int consumer_sum = 0;
int consumed = 0;
int produced = 0;

pthread_mutex_t m;
pthread_cond_t cv;

void *producer(void *threadid) {
  pthread_mutex_lock(&m);
  int num = (rand() % 10) + 1;
  printf("producer created %d\n", num);
  producer_buffer[produced++] = num;
  pthread_mutex_unlock(&m);
  pthread_cond_signal(&cv);
}

void *consumer(void *threadid) {
  while (consumed < PRODUCERS) {
    pthread_mutex_lock(&m);
    while (consumed >= produced) pthread_cond_wait(&cv, &m);
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
  pthread_exit(NULL);

}
