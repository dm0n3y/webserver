#ifndef __REQUEST_H__
#define __REQUEST_H__

typedef int status_t;

typedef struct http_request_t_ {
  int method;
  char path[MAXPATH];
  int httpver;
} http_request_t;

status_t parse_request(char *msg, http_request_t *request);

#endif // __REQUEST_H__
