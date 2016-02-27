#ifndef __REQUEST_H__
#define __REQUEST_H__

typedef int status_t;

typedef enum { GET, HEAD, POST } http_method_t;

typedef struct http_request_t_ {
  http_method_t method;
  char path[MAXPATH];
  int httpver;
} http_request_t;

status_t parse_request(char *msg, http_request_t *request);

#endif // __REQUEST_H__
