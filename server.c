#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define MAXLEN 200
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

int new_listening_socket() {
  int serverfd;
  struct sockaddr_in serveraddr;
  
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(8998);
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

  serverfd = Socket(AF_INET, SOCK_STREAM, 0);
  Bind(serverfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
  Listen(serverfd, 10);
  return serverfd;
}


void str_echo(int clientfd) {
  ssize_t n;
  char buf[MAXLEN];
  char quit[4] = {'q','u','i','t'};

  while ( (n = read(clientfd, buf, sizeof(buf))) > 0 ) {
    if (memcmp(buf,quit,4) == 0)
      close(clientfd);
    write(clientfd, buf, n);
  }
}

void *str_echo_routine(void *arg) {
  pthread_detach(pthread_self());
  str_echo((int)arg);
  close((int)arg);
  return NULL;
}


void *basic_routine(void *arg) {
  int clientfd = (int)arg;
  ssize_t n;  // length of received msg
  char buf[MAX_MSG_LEN];
  
  pthread_detach(pthread_self());

  // read in clientfd request
  n = Recv(clientfd, buf, sizeof(buf), 0);
  
  // parse initial request line (assuming correct)
  if 
  
  // write back initial request line
  // write back header line with time
  // write back content of requested file 1024 bytes at a time
}


int main() {

  pthread_t tid; // currently does not keep track of all created tids
  
  int serverfd, clientfd;
  struct sockaddr_in /*serveraddr,*/ clientaddr;  
  socklen_t clientlen;
  char buff[MAXLEN];

  serverfd = new_listening_socket();
  //memset(&clientaddr, 0, sizeof(clientaddr));
  
  while (1) {
    clientlen = sizeof(clientaddr);
    clientfd = Accept(serverfd, (struct sockaddr *) &clientaddr, &clientlen);
    printf("connection from %s, port %d\n",
	   inet_ntop(AF_INET, &clientaddr.sin_addr, buff, sizeof(buff)),
	   ntohs(clientaddr.sin_port));
    pthread_create(&tid, NULL, str_echo_routine, (void *)clientfd);
    //str_echo(clientfd);
    //close(clientfd);
  }
}
