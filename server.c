#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <time.h>
#include "util.h"
#include "queue.h"
#include "http.h"

/* Some wrapper functions for listening socket initalization,
 * where we may simply exit if we cannot set up the socket.
 */
int socket_(int domain, int type, int protocol) {
  int sockfd;
  if ( (sockfd = socket(domain,type,protocol)) < 0 ) {
    fprintf(stderr, "Socket error!\n");
    exit(1);
  }
  return sockfd;
}
int bind_(int socket, const struct sockaddr *address, socklen_t address_len) {
  int ret;
  if ( (ret = bind(socket,address,address_len)) < 0 ) {
    //fprintf(stderr, "Bind error (%d)!\n", ret);
    perror("bind_");
    exit(1);
  }
  return ret;
}
int listen_(int socket, int backlog) {
  int ret;
  if ( (ret = listen(socket,backlog)) < 0 ) {
    fprintf(stderr, "Listen error!\n");
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
  serveraddr.sin_port = htons(PORT);
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

  listenfd = socket_(AF_INET, SOCK_STREAM, 0);
  bind_(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
  listen_(listenfd, BACKLOG);
  return listenfd;
}

void *worker_routine(void *arg) {
  pthread_detach(pthread_self());
  
  int connfd, file, len, recv_bytes;
  char msg[MAXMSG], buf[1024], c;
  status_t status;
  http_request_t *request = malloc(sizeof(http_request_t));
  queue_t *q = (queue_t *)arg;
  struct stat st; // for file stats

#if DEBUG_ENQ
  int close_count;
  int reenqueue_count;
  int slowclose_count;
#endif

  while (1) {
  loopstart:
    dequeue(q,&connfd);
    memset(msg,0,MAXMSG); recv_bytes = 0;

    /* Loop until full HTTP msg is received */
    while (strstr(strndup(msg,recv_bytes),"\r\n\r\n") == NULL &&
	         strstr(strndup(msg,recv_bytes),"\n\n"    ) == NULL &&
           recv_bytes < MAXMSG) {
      if ( (len = recv(connfd,msg+recv_bytes,MAXMSG-recv_bytes,0)) <= 0 ) {
        //perror("recv");
        /* If client has closed, then close and move on */
        if (len == 0) {
#if DEBUG_ENQ
          printf("slow-closed %d (%d)\n",connfd,++slowclose_count);
#endif
          close(connfd);
          goto loopstart;
        }
        /* If timeout or error, skip parsing and send
         * appropriate error message */
        if (errno == EWOULDBLOCK) {
          status = REQUEST_TIMEOUT_;
          #if DEBUG_ERR
          printf("%d: timeout\n", connfd);
          #endif
        }
        else {
          status = SERVER_ERR_; perror("recv");
          #if DEBUG_ERR
          printf("%d: server error\n", connfd);
          #endif
        }
        goto send;
      }
      recv_bytes += len;
    }
    
    /* Parse (complete) message */
    status = parse_request(msg,request);

  send:
    /* Send initial line */
    len = sprintf(msg, "HTTP/1.%d %d %s\r\n",request->httpver,
		                             status,
		                             status_to_str(status));
    send(connfd,msg,len,0);

    /* Send header lines */
    time_t now;
    time(&now);
    len = strftime(buf,1024,
		   "Date: %a, %d %b %Y %H:%M:%S GMT\r\n",gmtime(&now));
    send(connfd,buf,len,0);
    if (status == OK_ && request->method == GET) {
      stat(request->path, &st);
      len = sprintf(msg, "Content-Length: %d\r\n", (int)st.st_size);
      send(connfd,msg,len,0);
      len = sprintf(msg, "Content-Type: %s\r\n", type_to_str(request->type));
      send(connfd,msg,len,0);
    }
    send(connfd,"\r\n",2,0);
    
    /* If request was well-formed GET, then send file */
    if (status == OK_ && request->method == GET) {
      if ( (file = open(request->path, O_RDONLY)) < 0 ) perror("open");
      while ( (len = read(file,msg,MAXMSG)) > 0 )
        if ( send(connfd,msg,len,0) < 0 ) perror("sending file");
      close(file);
    }

    /* If HTTP/1.0 or recv error, close connection. */
    if (request->httpver == 0 || status != OK_) {
      close(connfd);
      #if DEBUG_ENQ
      if (status != OK_) {
        printf("bad status: %d\n",status);
        ++close_count;
        if (close_count % 1000 == 0 || close_count > 10000) {
          printf("closed %d (%d)\n",connfd,close_count);
        }
      }
      #endif
    }
    /* Otherwise, keep connection alive and re-enqueue */
    else {
      enqueue(q,connfd);
      #if DEBUG_ENQ
      printf("re-enqueued %d (%d)\n", connfd, ++reenqueue_count);
      #endif
    }
  }
}

struct greeter_args {
  int listfd;
  queue_t *q;
};

void *greeter_routine(void *arg) {
  struct greeter_args *ga = (struct greeter_args *)arg;
  int listfd = ga->listfd;
  queue_t *q = ga->q;
  
  int connfd, n;
  struct sockaddr_in clientaddr;
  socklen_t clientlen;
  struct timeval timeout;

  /* For logging peername
  int res;
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(struct sockaddr_in);
  char clientip[20];
  */
  
  /* Accept connections, set their timeouts, and enqueue them */
  while(1) {
    clientlen = sizeof(clientaddr);
    connfd = accept(listfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (connfd < 0) {
      perror("accept");
      continue;
    }

    /* Get peername and log
    res = getpeername(connfd, (struct sockaddr *)&addr, &addr_size);
    strcpy(clientip, inet_ntoa(addr.sin_addr));
    printf("new connection: %s\n",clientip);
    */

    /* Basic heuristic for timeout based on queue length.
       Minimum timeout 10s + another second for every 50
       connections on the queue. */
    n = q->size;
    timeout.tv_sec = 10;
    if (n > 0) timeout.tv_sec += n/50;
    setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO,
               (void *)&timeout, sizeof(timeout));
#if DEBUG
      printf("connection from %s, port %d\n",
             inet_ntop(AF_INET, &clientaddr.sin_addr, buf, sizeof(buf)),
             ntohs(clientaddr.sin_port));
      printf("%d\n",q->size);
#endif
    enqueue(q,connfd);
  }
}

// default DOCUMENT_ROOT for my own use
// const char *DOCUMENT_ROOT = "/home/cs-students/15dm7/cs339/a1/resources";
const char *DOCUMENT_ROOT;
int PORT;
int main(int argc, char *argv[]) {
#if DEBUG
  char buf[1024];
#endif

  int listfd, connfd, i;
  queue_t *connections;
  pthread_t workers[NUM_THREADS/2];
  pthread_t greeters[NUM_THREADS/2];
  //struct sockaddr_in clientaddr;
  //socklen_t clientlen;
  //struct timeval timeout;
  //timeout.tv_usec = 0;

  /* Get current working directory */
  char cwd[1024];
  const char *RESOURCES = "/resources";
  if (getcwd(cwd, sizeof(cwd)-sizeof(RESOURCES)) == NULL) {
    perror("getcwd() error");
  }
  /* Assign document root */
  DOCUMENT_ROOT = strcat(cwd,RESOURCES);

  /* Parse command line args */
  i = 1;
  if (i < argc-1 && strcmp(argv[i],"-document_root") == 0) {
    DOCUMENT_ROOT = argv[++i]; ++i;
  }
  if (i < argc-1 && strcmp(argv[i],"-port") == 0) {
    PORT = atoi(argv[i+1]);
  } else {
    printf("Usage: ./server [-document_root <path>] -port <portno>\n");
    return 0;
  }
  
  /* Initalize connections queue */
  connections = malloc(sizeof(queue_t));
  queue_init(connections);

  /* Initialize listening socket */
  listfd = listening_socket();

  /* Package arguments for greeter threads */
  struct greeter_args ga = { .listfd = listfd, .q = connections };

  /* Spawn greeter threads. */
  for (i = 0; i < NUM_THREADS/2; i++)
    pthread_create(&greeters[i], NULL, greeter_routine, (void *)(&ga));

  /* Spawn worker threads. These will immediately
   * block until signaled by main server thread 
   * pushes connections onto the queue and signals. */
  for (i = 0; i < NUM_THREADS/2; i++)
    pthread_create(&workers[i], NULL, worker_routine, (void *)connections);

  pthread_exit(NULL);

  //queue_destroy(connections);
  //free(connections);
}
