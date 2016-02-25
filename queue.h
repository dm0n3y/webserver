#ifndef __QUEUE_H__
#define __QUEUE_H__

typedef struct node_t_ {
  int fd;
  struct node_t_ *next;
} node_t;

typedef struct queue_t_ {
  node_t *head;
  node_t *tail;
  pthread_mutex_t *hlock;
  pthread_mutex_t *tlock;
  pthread_cond_t *nonempty;
} queue_t;

void queue_init(queue_t *q);
void enqueue(queue_t *q, int fd);
int dequeue(queue_t *q, int *fd);

#endif // __QUEUE_H__