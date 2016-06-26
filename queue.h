#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "util.h"

typedef struct node_t_ {
  int fd;
  struct node_t_ *next;
} node_t;

typedef struct queue_t_ {
  node_t *head;
  node_t *tail;
  pthread_mutex_t *hlock; /* guards head */
  pthread_mutex_t *tlock; /* guards tail */
#if DEBUG_QSIZE
  pthread_mutex_t *slock; /* guards size */
#endif
  pthread_cond_t *nonempty;
  int size; /* imprecise and racy, only used
             * for connection timeout heuristic */

#if DEBUG_ENQ
  int enq_count;
#endif
} queue_t;

void queue_init(queue_t *q);
void queue_destroy(queue_t *q);
void enqueue(queue_t *q, int fd);
void dequeue(queue_t *q, int *fd);

#endif // __QUEUE_H__
