#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "queue.h"

void queue_init(queue_t *q) {
  node_t *dummy = malloc(sizeof(node_t));
  pthread_mutex_t *hlock = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_t *tlock = malloc(sizeof(pthread_mutex_t));
  pthread_cond_t *nonempty = malloc(sizeof(pthread_cond_t));
  //pthread_mutex_t *hlock, *tlock;
  if ( pthread_mutex_init(hlock,NULL) != 0 ||
       pthread_mutex_init(tlock,NULL) != 0 )
  {
    free(dummy); free(hlock); free(tlock); free(nonempty);
    fprintf(stderr, "Could not initialize mutex locks for queue!");
    exit(1);
  }

  dummy->next = NULL;
  q->head = (q->tail = dummy);
  q->hlock = hlock;
  q->tlock = tlock;
  q->nonempty = nonempty;

  pthread_cond_init(q->nonempty, NULL);
}

void queue_destroy(queue_t *q) {
  node_t *node = q->head;
  node_t *next;
  while (node != NULL) {
    next = node->next;
    free(node);
    node = next;
  }
  free(q->hlock); free(q->tlock);
  free(q->nonempty);
  free(q);
}

void enqueue(queue_t *q, int fd) {
  printf("enqueue\n");  
  node_t *node = malloc(sizeof(node_t));
  node->fd = fd;
  node->next = NULL;

  pthread_mutex_lock(q->tlock);
  q->tail->next = node;
  q->tail = node;
  pthread_mutex_unlock(q->tlock);
  pthread_cond_signal(q->nonempty);
}

void dequeue(queue_t *q, int *fd) {
  node_t *dummy, *new_dummy;
  pthread_mutex_lock(q->hlock);
  dummy = q->head;
  new_dummy = dummy->next;
  /*
  if (new_head == NULL) {
    pthread_mutex_unlock(q->hlock);
    return -1;
  }
  */
  printf("dequeue %d: before if\n", ((int)fd)%1000);  
  if (new_dummy == NULL) {
    printf("dequeue %d: inside if\n", ((int)fd)%1000);
    pthread_cond_wait(q->nonempty, q->hlock);
    new_dummy = dummy->next;
  }
  printf("dequeue %d: after if\n", ((int)fd)%1000);
  
  *fd = new_dummy->fd;
  pthread_mutex_unlock(q->hlock);
  printf("dequeue %d: before free\n", ((int)fd)%1000);
  free(dummy);
  printf("dequeue %d: after free\n", ((int)fd)%1000);
  //return 0;
}

typedef struct iq_pair_ {
  int i;
  queue_t *q;
} iq_pair;

void *test_enqueue(void *arg) {
  //pthread_detach(pthread_self());
  iq_pair *iq = (iq_pair *)arg;
  enqueue(iq->q, iq->i);
  printf("Enqueued %d\n",iq->i);
  return NULL;
}

void *test_dequeue(void *arg) {
  queue_t *q = (queue_t *)arg;
  int fd;
  dequeue(q,&fd);
  printf("Dequeued %d (%d)\n",fd,((int)&fd)%1000);
  return NULL;
}

int main() {
  queue_t *q = malloc(sizeof(queue_t));
  queue_init(q);
  int j;
  iq_pair *iq;
  
  int n = 10;
  pthread_t enq_tids[n];
  pthread_t deq_tids[n];
  iq_pair *iqs[n];

  for (j = 0; j < n; j++) {
    iqs[j] = malloc(sizeof(iq_pair));
    iqs[j]->i = j;  iqs[j]->q = q;
  }
  for (j = 0; j < n; j++) {
    pthread_create(&deq_tids[n], NULL, test_dequeue, (void *)q);
  }  
  for (j = 0; j < n; j++) {
    pthread_create(&enq_tids[j], NULL, test_enqueue, (void *)iqs[j]);
    free(iq);
  }

  int *ret;
  for (j = 0; j < n; j++) {
    pthread_join(enq_tids[j], (void *)&ret);
    free(iqs[j]);
  }

  queue_destroy(q);
}
