/*******************************************************************
* prod-con-threads.c
* Producer Consumer With C
* Compile: gcc -pthread -o prodcont prod-con-threads.c
* Run: ./prodcont
*******************************************************************/
#include <errno.h>          /* errno, ECHILD            */
#include <pthread.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>          /* O_CREAT, O_EXEC          */

#define PRODUCERS  2
#define CONSUMERS  1
#define MAX_BUFFER_SIZE 10

int *producer_buffer; // pointer to an array
int *consumer_sum;
sem_t *m;             // binary semaphore, acting as mutex
sem_t *available;     // counting semaphore, indicate number of consumables
int *produced;         // next index to produce on
int *consumed;         // next index to consume from

void *producer() {
  srand(getpid());
  sem_wait(m);
  int num = (rand() % 10) + 1;
  printf("producer created %d\n", num);
  producer_buffer[(*produced)++] = num;
  sem_post(m);
  sem_post(available);
}

void *consumer() {
  srand(time(NULL) + 1);
  // Assumes only 1 consumer.
  while (*consumed < PRODUCERS) {
    sem_wait(available);
    sem_wait(m);
    *consumer_sum += producer_buffer[(*consumed)++];
    sem_post(m);
  }
}

int main(int argc, char *argv[]) {
  // allocate shared memory
  key_t cs_shmkey = ftok("/dev/null", 5);
  int cs_shmid = shmget(cs_shmkey, sizeof(int), 0644 | IPC_CREAT);
  consumer_sum = (int *) shmat(cs_shmid, NULL, 0);
  *consumer_sum = 0;

  key_t p_shmkey = ftok("/dev/null", 6);
  int p_shmid = shmget(p_shmkey, sizeof(int), 0644 | IPC_CREAT);
  produced = (int *) shmat(p_shmid, NULL, 0);
  *produced = 0;

  key_t c_shmkey = ftok("/dev/null", 7);
  int c_shmid = shmget(c_shmkey, sizeof(int), 0644 | IPC_CREAT);
  consumed = (int *) shmat(c_shmid, NULL, 0);
  *consumed = 0;

  key_t b_shmkey = ftok("/dev/null", 8);
  int b_shmid = shmget(b_shmkey, sizeof(int) * MAX_BUFFER_SIZE, 0644 | IPC_CREAT);
  producer_buffer = (int *) shmat(b_shmid, NULL, 0);

  // allocate semaphores
  m = sem_open("mSem", O_CREAT | O_EXCL, 0644, 1);
  available = sem_open("availSem", O_CREAT | O_EXCL, 0644, 0);

  long t1, t2;
  for (t1 = 0; t1 < PRODUCERS; t1++) {
    printf("creating producer %d\n", t1);
    if (!fork()) {
      // Child proc
      producer();
      exit(0);
    }
  }

  for (t2 = 0; t2 < CONSUMERS; t2++) {
    printf("creating consumer %d\n", t2);
    if (!fork()) {
      consumer();
      exit(0);
    }
  }

  pid_t pid;
  /* wait for all children to exit */
  while (pid = waitpid(-1, NULL, 0)) {
    if (errno == ECHILD) break;
  }

  printf("### consumer_sum final value = %d ###\n", *consumer_sum);

  // cleanup shared memory
  shmdt(consumer_sum);
  shmctl(cs_shmid, IPC_RMID, 0);

  shmdt(produced);
  shmctl(p_shmid, IPC_RMID, 0);

  shmdt(consumed);
  shmctl(c_shmid, IPC_RMID, 0);

  shmdt(producer_buffer);
  shmctl(b_shmid, IPC_RMID, 0);

  // cleanup semaphores
  sem_unlink("mSem");
  sem_unlink("availSem");
  sem_close(m);
  sem_close(available);

  pthread_exit(NULL);
}
