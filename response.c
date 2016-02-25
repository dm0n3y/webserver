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

// arbitrary cap for now
#define MAXLINE 4096

char *status_to_phrase(int status) {
  switch (status) {
    case OK_:
      return "OK";
    case BAD_REQUEST_:
      return "Bad Request";
    case FORBIDDEN_:
      return "Forbidden";
    case NOT_FOUND_:
      return "Not Found";
    case REQUEST_TOO_LARGE_:
      return "Request Entity Too Large";
    case SERVER_ERR_:
    default:
      return "Internal Server Error";
  }
}

void send_initial(int clientfd, int status) {
  char buf[MAXLINE];
  int len = sprintf(buf,"HTTP/1.0 %d %s\n",status,status_to_phrase(status));
  send(clientfd, buf, len, 0);  
  // Later, should account for possible error in system calls and re-send
  // a 500 response, but send at most one.
  // This will require keeping count per thread, which might be annoying
  // synchronization-wise... Actually, no, not at all. Yay.
}

void send_date_header(int clientfd) {
  char *s = "It's Tuesday, bitch.\n";
  send(clientfd, s, strlen(s), 0);
}

void send_contenttype_header(int clientfd) {
  char *s = "It's HTML, bitch.\n";
  send(clientfd, s, strlen(s), 0);
}

void send_contentlen_header(int clientfd) {
  char *s = "It's not as big as your mom, bitch.\n";
  send(clientfd, s, strlen(s), 0);
}

void send_content(int clientfd) {
  
}

void send_response(int clientfd, int status) {
  send_initial(clientfd, status);
  send_date_header(clientfd);
  send_contenttype_header(clientfd);
  send_contentlen_header(clientfd);
  // send content
}

