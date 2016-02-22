#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>

typedef struct locked_data_ {
  int n;
  pthread_mutex_t *lock;
} locked_data;

void init(locked_data *d, pthread_mutex_t *lock) {
  d->n = 0;
  pthread_mutex_init(lock,NULL);
  d->lock = lock;
}

int read(locked_data *d) {
  int n;
  //pthread_mutex_lock(d->lock);
  n = d->n;
  //pthread_mutex_unlock(d->lock);
  return n;
}

void inc(locked_data *d) {
  //pthread_mutex_lock(d->lock);
  (d->n)++;
  //pthread_mutex_unlock(d->lock);
}

void *printdata_routine(void *data) {
  locked_data *d = (locked_data *)data;
  pthread_mutex_lock(d->lock);
  printf("read %d\n",read(d));
  inc(d);
  pthread_mutex_unlock(d->lock);
  return 0;
}

int main() {
  int n = 10;
  pthread_t threads[n];
  void *status;

  locked_data *data = malloc(sizeof(locked_data));
  pthread_mutex_t *lock = malloc(sizeof(pthread_mutex_t));
  init(data,lock);

  int i;
  for (i = 0; i < n; i++) {
    pthread_create(&(threads[i]), NULL, printdata_routine, data);
  }
  for (i = 0; i < n; i++) {
    pthread_join(threads[i], &status);
    printf("thread[%d] return status: %d\n",i,(int)status);
  }
}
