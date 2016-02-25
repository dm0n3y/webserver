#include <stdlib.h>
#include <pthread.h>
#include "queue.h"

void queue_init(queue_t *q) {
  node_t *dummy = malloc(sizeof(node_t));
  pthread_mutex_t *hlock, *tlock;
  if ( pthread_mutex_init(hlock,NULL) != 0 ||
       pthread_mutex_init(tlock,NULL) != 0 )
  {
    fprintf(stderr, "Could not initialize mutex locks for queue!");
    exit(1);
  }
  
  dummy->next = NULL;
  q->head = (q->tail = dummy);
  q->hlock = hlock;
  q->tlock = tlock;

  pthread_cond_init(q->nonempty, NULL);
}

void enqueue(queue_t *q, int fd) {
  node_t *node = malloc(sizeof(node_t));
  node->fd = fd;
  node->next = NULL;
  
  pthread_mutex_lock(q->tlock);
  q->tail->next = node;
  q->tail = node;
  pthread_mutex_unlock(q->tlock);
}

int dequeue(queue_t *q, int *fd) {
  node_t *dummy, *new_dummy;
  pthread_mutex_lock(q->hlock);
  dummy = q->head;
  new_dummy = dummy->next;

  if (new_head == NULL) {
    pthread_mutex_unlock(q->hlock);
    return -1;
  }
  /*
  if (new_dummy == NULL) {
    pthread_cond_wait(q->nonempty);
    new_dummy = dummy->next;
  }
  */
  
  *fd = new_dummy->fd;
  pthread_mutex_unlock(q->hlock);
  free(dummy);
  return 0;
}



int main() {
  queue_t *q = malloc(sizeof(q));
}
