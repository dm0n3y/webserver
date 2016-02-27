#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <bsd/string.h>
#include <fcntl.h>
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

#define MAXLEN 8192
#define MAX_MSG_LEN 1024

/* I borrow Stevens et al's convention of wrapping error handling for
 * a system call in a function with the same name, but the first letter
 * capitalized.
 */
/*
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
*/

/* Initialize listening socket.
 */
int listening_socket() {
  int listenfd;
  struct sockaddr_in serveraddr;
  
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(PORT_);
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  bind(listenfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
  listen(listenfd, BACKLOG_);
  return listenfd;
}

/*
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
*/
/*
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
*/

void *worker_routine(void *arg) {
  pthread_detach(pthread_self());
  
  int connfd, file, len, recv_bytes;
  char msg[MAXMSG], buf[1024], c;
  status_t status;
  http_request_t *request = malloc(sizeof(http_request_t));
  queue_t *q = (queue_t *)arg;
  struct stat st; // for file stats
  
  while (1) {
  loopstart:
    dequeue(q,&connfd);
    memset(msg,0,MAXMSG); recv_bytes = 0;
    while (strstr(strndup(msg,recv_bytes),"\r\n\r\n"/*,MAXMSG*/) == NULL &&
	   strstr(strndup(msg,recv_bytes),"\n\n"    /*,MAXMSG*/) == NULL &&
           recv_bytes < MAXMSG) {
      /* If msg too large, skip parsing and send response */
      /*
      if (recv_bytes >= MAXMSG) {
	
	status = REQUEST_TOO_LARGE_;
	goto send;
      }
      */
      /*len = recv(connfd,msg+recv_bytes,MAXMSG-recv_bytes,0); */
      /* If client has closed, close and move on */
      /*
      if (len == 0) {
	close(connfd);
	goto loopstart;
	}*/
      /* If socket has timed out, skip parsing and send response */
      /*
      if (len < 0) {
	perror("recv");
	status = REQUEST_TIMEOUT_;
	goto send;
	} */

      
      /* If error or time out, close and move on */
      if ( (len = recv(connfd,msg+recv_bytes,MAXMSG-recv_bytes,0)) <= 0 ) {
	if (len < 0) perror("recv");
	close(connfd);
	goto loopstart;
      }
      recv_bytes += len;
    }
    //printf("Received: %s\n",msg);
    int i;
    for (i = 0; i < len; i++) putchar(msg[i]); //putchar('\n');
    status = parse_request(msg,request);
    //printf("Parse result: %d\n",status);

  send:
    /* Send initial line */
    // ADD REASON PHRASE
    len = sprintf(msg, "HTTP/1.%d %d\r\n",request->httpver,status);
    send(connfd,msg,len,0);

    /* Send header lines */
    time_t now;
    time(&now);
    len = strftime(buf,1024,
		   "Date: %a, %d %b %Y %H:%M:%S GMT\r\n",gmtime(&now));
    send(connfd,buf,len,0);

    if (status == OK_ && request->method == GET) {
      // ADD ERROR HANDLING
      stat(request->path, &st);
      len = sprintf(msg, "Content-Length: %d\r\n", (int)st.st_size);
      send(connfd,msg,len,0);
      len = sprintf(msg, "Content-Type: %s\r\n", request->type);
    }

    /* Send empty line */
    send(connfd,"\r\n",2,0);
    
    // if get, send file  [ BUT NOT IF PARSE RETURNED ERROR! ]
    if (status == OK_ && request->method == GET) {     
      if ( (file = open(request->path, O_RDONLY)) < 0 ) {
	perror("open");
	printf("%s\n",request->path);
      }
      while ( (len = read(file,msg,MAXMSG)) > 0 ) {
	// ADD ERROR HANDLING
	//printf("sending %s\n",request->path);
	/*
	printf("sending\n");
	for (i = 0; i < len; i++) putchar(msg[i]); //putchar('\n');
	printf("\n");
	*/
	if ( send(connfd,msg,len,0) < 0 ) perror("sending file");
      }
      close(file);
      printf("sent %s\n",request->path);
    }

    if (request->httpver == 0)
      close(connfd);
    else
      enqueue(q,connfd);
  }
}


int main() {

  char buf[1024]; // for DEBUG_PRINT in loop
  
  int listfd, connfd, i;
  queue_t *connections;
  pthread_t workers[NUM_WORKERS];  
  struct sockaddr_in clientaddr;
  socklen_t clientlen;
  struct timeval timeout;
  timeout.tv_sec  = 10;
  timeout.tv_usec = 0;

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
    connfd = accept(listfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (connfd < 0) {
      perror("accept");
      continue;
    }
    setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO,
	       (void *)&timeout, sizeof(timeout));
    /*
    DEBUG_PRINT("connection from %s, port %d\n",
	        inet_ntop(AF_INET, &clientaddr.sin_addr, buf, sizeof(buf)),
	        ntohs(clientaddr.sin_port));
    */
    enqueue(connections,connfd);
  }

  queue_destroy(connections);
  free(connections);
  
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

  
  
  /*
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
  */
}
