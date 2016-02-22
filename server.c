#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

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


int main() {

  int serverfd, clientfd;
  socklen_t len;
  char buff[200];
  struct sockaddr_in serveraddr, clientaddr;

  memset(&clientaddr, 0, sizeof(clientaddr));
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(8998);
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  
  serverfd = Socket(AF_INET, SOCK_STREAM, 0);
  Bind(serverfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr));
  Listen(serverfd, 10);
  while (1) {
    len = sizeof(clientfd);
    Accept(serverfd, (struct sockaddr *) &clientaddr, &len);
    printf("connection from %s, port %d\n",
	   inet_ntop(AF_INET, &clientaddr.sin_addr, buff, sizeof(buff)),
	   ntohs(clientaddr.sin_port));
    close(clientfd);
  }
}
