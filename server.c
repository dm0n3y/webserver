#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "util.h"
#include "response.h"

#define MAXLEN 8192
#define MAX_MSG_LEN 1024

/* I borrow Stevens et al's convention of wrapping error handling for
 * a system call in a function with the same name, but the first letter
 * capitalized.
 */
int Socket(int domain, int type, int protocol) {
  int sockfd;
  if ( (sockfd = socket(domain,type,protocol)) < 0 ) {
    fprintf(stderr, "Socket error!\n");
    exit(1);
  }
  return sockfd;
}
int Bind(int socket, const struct sockaddr *address, socklen_t address_len) {
  int ret;
  if ( (ret = bind(socket,address,address_len)) < 0 ) {
    fprintf(stderr, "Bind error!\n");
    exit(1);
  }
  return ret;
}
int Listen(int socket, int backlog) {
  int ret;
  if ( (ret = listen(socket,backlog)) < 0 ) {
    fprintf(stderr, "Listen error!\n");
    exit(1);
  }
  return ret;
}
int Accept(int socket, struct sockaddr *restrict address, socklen_t *restrict address_len) {
  int ret;
  if ( (ret = accept(socket,address,address_len)) < 0 ) {
    fprintf(stderr, "Accept error!\n");
    exit(1);
  }
  return ret;
}
int Recv(int socket, void *buffer, size_t length, int flags) {
  int ret;
  if ( (ret = recv(socket,buffer,length,flags)) < 0 ) {
    fprintf(stderr, "Recv error!\n");
    exit(1);
  }
  return ret;
}

/* Initialize listening socket.
 */
int listening_socket() {
  int listenfd;
  struct sockaddr_in serveraddr;
  
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(PORT_);
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

  listenfd = Socket(AF_INET, SOCK_STREAM, 0);
  Bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
  Listen(listenfd, BACKLOG_);
  return listenfd;
}


void str_echo(int connfd) {
  size_t n;
  char buf[MAXLEN];
  char quit[4] = {'q','u','i','t'};

  while ( (n = read(connfd, buf, sizeof(buf))) > 0 ) {
    if (memcmp(buf,quit,4) == 0)
      close(connfd);
    DEBUG_PRINT("%s",buf);
    send(connfd, buf, n, 0);
  }
}

void *str_echo_routine(void *arg) {
  pthread_detach(pthread_self());
  str_echo((int)arg);
  close((int)arg);
  return NULL;
}


void *basic_routine(void *arg) {
  int connfd = (int)arg;
  ssize_t n;  // length of received msg
  char buf[MAX_MSG_LEN];
  
  pthread_detach(pthread_self());

  // read in connfd request
  n = Recv(connfd, buf, sizeof(buf), 0);
  
  // parse initial request line (assuming correct)
  // write back initial request line
  // write back header line with time
  // write back content of requested file 1024 bytes at a time
  send_response(connfd, OK_);
  close(connfd);
  return NULL;
}

void *worker_routine(void *arg) {
  pthread_detach(pthread_self());
  
  int connfd;
  char msg[MAXMSG];
  http_request_t *request = malloc(sizeof(http_request_t));
  queue_t *q = (queue_t *)arg;
  
  while (1) {
    dequeue(q,&connfd);
    recv(connfd,msg,MAXMSG,MSG_WAITALL);
    parse_request(msg,request);

    if (request->method == GET) {
      
    }
  }
}


int main() {

  int listfd, connfd, i;
  queue_t *connections;
  pthread_t workers[NUM_WORKERS];

  // Not sure if I need to access these later.
  // Remove if not.
  struct sockaddr_in clientaddr;
  socklen_t clientlen;

  /* Initalize connections queue */
  connections = malloc(sizeof(queue_t));
  queue_init(connections);

  /* Spawn worker threads. These will immediately
   * block until signaled by main server thread 
   * pushes connections onto the queue and signals. */
  for (i = 0; i < NUM_WORKERS; i++)
    pthread_create(&workers[i], NULL, worker_routine, (void *)connections);
  
  listfd = listening_socket();

  while(1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);
    DEBUG_PRINT("connection from %s, port %d\n",
	        inet_ntop(AF_INET, &clientaddr.sin_addr, buf, sizeof(buf)),
	        ntohs(clientaddr.sin_port));
    enqueue(q,connfd);
  }

  /*
    Ah, so I might need two queues:
    - first is simply to accept incoming connections
    - second stores http_request_t structs
      - there need to be threads that pop from q1, read and create struct,
        push onto q2
      - threads at end of q2 pull off structs
        - if done, send response
        - if not...

     Scratch that. Just one:
     - main server thread just mallocs request struct and enqueues
     - worker threads pop off structs
       - call recv on the connection
       - if timed out or closed, get rid of it

     Let's just assume that a worker thread should read until it encounters
     double \n\n

     What the hell, let's just do WAITALL for now
     In this case, worker threads just block on recv until entire message has
     arrived, then starts parsing.
     If the socket had timed out before then, then just...

     
  */



  queue_destroy(q);
  free(q);
  
  
  
  int listenfd, connfd;
  struct sockaddr_in clientaddr;  
  socklen_t clientlen;
  char buf[MAXLEN];

  listenfd = new_listening_socket();
  //memset(&clientaddr, 0, sizeof(clientaddr));
  
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (struct sockaddr *) &clientaddr, &clientlen);
    DEBUG_PRINT("connection from %s, port %d\n",
	        inet_ntop(AF_INET, &clientaddr.sin_addr, buf, sizeof(buf)),
	        ntohs(clientaddr.sin_port));
    //pthread_create(&tid, NULL, basic_routine, (void *)connfd);
    //str_echo(connfd);
    close(connfd);
  }
}
