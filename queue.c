#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include "queue.h"
#include "util.h"

void queue_init(queue_t *q) {

  /*
   * Come back and rewrite this as nested ifs.
   */
  
  node_t *dummy = malloc(sizeof(node_t));
  pthread_mutex_t *hlock = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_t *tlock = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_t *slock = malloc(sizeof(pthread_mutex_t));
  pthread_cond_t *nonempty = malloc(sizeof(pthread_cond_t));
  //pthread_mutex_t *hlock, *tlock;
  if ( pthread_mutex_init(hlock,NULL) != 0 ||
       pthread_mutex_init(tlock,NULL) != 0 ||
       pthread_mutex_init(slock,NULL) )
  {
    free(dummy); free(hlock); free(tlock); free(slock); free(nonempty);
    fprintf(stderr, "Could not initialize mutex locks for queue!");
    exit(1);
  }

  dummy->next = NULL;
  q->head = (q->tail = dummy);
  q->hlock = hlock;
  q->tlock = tlock;
  q->slock = slock;
  q->nonempty = nonempty;
  pthread_cond_init(q->nonempty, NULL);

  q->size = 0;
}

void queue_destroy(queue_t *q) {
  node_t *node = q->head;
  node_t *next;
  while (node != NULL) {
    next = node->next;
    free(node);
    node = next;
  }
  free(q->hlock); free(q->tlock); free(q->slock);
  free(q->nonempty);
  free(q);
}

void enqueue(queue_t *q, int fd) {
  //DEBUG_PRINT("enqueue\n");
  node_t *node = malloc(sizeof(node_t));
  node->fd = fd;
  node->next = NULL;

  pthread_mutex_lock(q->tlock);
  q->tail->next = node;
  q->tail = node;
  DEBUG_PRINT("\t\t\t\t\t| enqueued %p { fd = %d }\n\t\t\t\t\t|    dummy %p { fd = %d }\n",node,fd,q->head,q->head->fd);
  pthread_cond_signal(q->nonempty);
  pthread_mutex_unlock(q->tlock);
}

void dequeue(queue_t *q, int *fd) {
  node_t *dummy, *new_dummy;
  pthread_mutex_lock(q->hlock);
  //dummy = q->head;
  //new_dummy = dummy->next;
  /*
  if (new_head == NULL) {
    pthread_mutex_unlock(q->hlock);
    return -1;
  }
  */
  DEBUG_PRINT("[dq %p] before loop (%p,%p)\n", fd,dummy,new_dummy);//((int)fd)%1000);  
  while (q->head->next == NULL) {
    //    DEBUG_PRINT("[dq %p] inside loop, dummy: node %p { fd = %d }\n", fd, q->head, q->head->fd);//((int)fd)%1000);
    pthread_cond_wait(q->nonempty, q->hlock);

    //dummy = q->head;
    //new_dummy = dummy->next;
    //DEBUG_PRINT("[dq %p] inside loop (%p,%p)\n", fd,dummy,new_dummy);
  }
  //DEBUG_PRINT("[dq %p] after while\n", fd);//((int)fd)%1000);
  dummy = q->head;
  new_dummy = dummy->next;
  DEBUG_PRINT("[dq %p] ready    %p { fd = %d }\n",fd,new_dummy,new_dummy->fd);
  
  *fd = new_dummy->fd;
  q->head = new_dummy;
  pthread_mutex_unlock(q->hlock);
  //DEBUG_PRINT("[dq %p] before free\n", fd);//((int)fd)%1000);
  free(dummy);
  //DEBUG_PRINT("[dq %p] after free\n", fd);//((int)fd)%1000);
  DEBUG_PRINT("[dq %p] dequeued %p { fd = %d }\n",fd,new_dummy,new_dummy->fd);
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
  //  DEBUG_PRINT("Enqueued %d\n",iq->i);
  return NULL;
}

void *test_dequeue(void *arg) {
  queue_t *q = (queue_t *)arg;
  int fd;
  dequeue(q,&fd);
  //  DEBUG_PRINT("Dequeued %d (%d)\n",fd,((int)&fd)%1000);
  printf("[dq %p] about to return\n",&fd);
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
    pthread_create(&deq_tids[j], NULL, test_dequeue, (void *)q);
  }  
  for (j = 0; j < n; j++) {
    pthread_create(&enq_tids[j], NULL, test_enqueue, (void *)iqs[j]);
    free(iq);
  }

  int *ret;
  int e;
  for (j = 0; j < n; j++) {
    e = pthread_join(enq_tids[j], (void *)&ret);
    if (e == EDEADLK)
      printf("enq thread %d is deadlocked!\n",j);
    else if (e == EINVAL)
      printf("enq thread %d is not joinable!\n",j);
    else if (e == ESRCH)
      printf("enq thread %d could not be found!\n",j);
    else if (e == 0)
      printf("enq thread %d was joined successfully!\n",j);
    else
      printf("enq thread %d is ???\n",j);
  }
  for (j = 0; j < n; j++) {
    e = pthread_join(deq_tids[j], (void *)&ret);
    if (e == EDEADLK)
      printf("deq thread %d is deadlocked!\n",j);
    else if (e == EINVAL)
      printf("deq thread %d is not joinable!\n",j);
    else if (e == ESRCH)
      printf("deq thread %d could not be found!\n",j);
    else if (e == 0)
      printf("deq thread %d was joined successfully!\n",j);
    else
      printf("deq thread %d is ???\n",j);
  }
  for (j = 0; j < n; j++) free(iqs[j]);

  
  queue_destroy(q);
}
