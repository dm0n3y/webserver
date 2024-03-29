#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include "queue.h"
#include "util.h"

/* Simple two-lock concurrent queue based on the one in
 * http://www.cs.rochester.edu/~scott/papers/1996_PODC_queues.pdf
 * The queue uses a head lock to protect concurrent dequeues
 * and a tail lock to protect concurrent enqueues. Enqueue always
 * succeeds, and their dequeue method may do nothing and return
 * false if it sees that the queue may be empty. My queue has an
 * extra wait-and-signal mechanism that allows dequeueing threads
 * to sleep while waiting for the queue to become nonempty.
 */

void queue_init(queue_t *q) {
  node_t *dummy;
  pthread_mutex_t *hlock, *tlock;
  pthread_cond_t *nonempty;

  if ( !(dummy = malloc(sizeof(node_t))) ) {
    fprintf(stderr, "Out of memory!\n");
    goto exit;
  }
  if ( !(hlock = malloc(sizeof(pthread_mutex_t))) ) {
    fprintf(stderr, "Out of memory!\n");
    goto cleanup_dummy;
  }
  if ( !(tlock = malloc(sizeof(pthread_mutex_t))) ) {
    fprintf(stderr, "Out of memory!\n");
    goto cleanup_hlock;
  }
  if ( !(nonempty = malloc(sizeof(pthread_cond_t))) ) {
    fprintf(stderr, "Out of memory!\n");
    goto cleanup_tlock;
  }
  if ( pthread_mutex_init(hlock,NULL) != 0 ||
       pthread_mutex_init(tlock,NULL) != 0  ) {
    fprintf(stderr, "Could not initialize mutex locks for queue!\n");
    goto cleanup_nonempty;
  }

  dummy->next = NULL;
  q->head = (q->tail = dummy);
  q->hlock = hlock;
  q->tlock = tlock;
  q->nonempty = nonempty;
  pthread_cond_init(q->nonempty, NULL);

  q->size = 0;
#if DEBUG_QSIZE
  pthread_mutex_t *slock = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(slock,NULL);
  q->slock = slock;
#endif
#if DEBUG_ENQ
  q->enq_count = 0;
#endif
  return;

 cleanup_nonempty:
  free(nonempty);
 cleanup_tlock:
  free(tlock);
 cleanup_hlock:
  free(hlock);
 cleanup_dummy:
  free(dummy);
 exit:
  exit(1);
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
#if DEBUG_QSIZE
  free(q->slock);
#endif
  free(q->nonempty);
}

void enqueue(queue_t *q, int fd) {
  /* Construct new node */
  node_t *node = malloc(sizeof(node_t));
  node->fd = fd;
  node->next = NULL;

  pthread_mutex_lock(q->tlock);
  {
    /* Add node to end of queue */
    q->tail->next = node;
    q->tail = node;
#if DEBUG_QSIZE
    pthread_mutex_lock(q->slock);
#endif
    q->size++;
#if DEBUG_QSIZE
    pthread_mutex_unlock(q->slock);
#endif
#if DEBUG_ENQ
    ++(q->enq_count);
    if (q->enq_count % 1000 == 0 ||
        q->enq_count > 10000)
      printf("enqueue count: %d\n",++(q->enq_count));
#endif

    /* Wake any sleeping worker threads */
    pthread_cond_signal(q->nonempty);
  }
  pthread_mutex_unlock(q->tlock);
}

void dequeue(queue_t *q, int *fd) {
  node_t *old_head;
  pthread_mutex_lock(q->hlock);
  {
    /* Wait until signaled that queue is nonempty.
     * Need while loop in case a new thread manages to
     * steal the queue element after the waiting thread
     * is signaled, but before it can reacquire hlock. */
    while (q->head->next == NULL) // i.e. q is empty
      pthread_cond_wait(q->nonempty, q->hlock);

    /* Store dequeued value and update dummy head */
    old_head = q->head;
    *fd = old_head->next->fd;
    q->head = q->head->next;
#if DEBUG_QSIZE
    pthread_mutex_lock(q->slock);
#endif
    q->size--;
#if DEBUG_QSIZE
    pthread_mutex_unlock(q->slock);
#endif
  }
  pthread_mutex_unlock(q->hlock);
  free(old_head);
}




/* Debugging */

typedef struct iq_pair_ {
  int i;
  queue_t *q;
} iq_pair;

void *test_enqueue(void *arg) {
  iq_pair *iq = (iq_pair *)arg;
  enqueue(iq->q, iq->i);
  return NULL;
}

void *test_dequeue(void *arg) {
  queue_t *q = (queue_t *)arg;
  int fd;
  dequeue(q,&fd);
  return NULL;
}

/*
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
  free(q);
}
*/
