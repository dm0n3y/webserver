#ifndef __REQUEST_H__
#define __REQUEST_H__

typedef int status_t;
#define OK_                200
#define BAD_REQUEST_       400
#define FORBIDDEN_         403
#define NOT_FOUND_         404
#define REQUEST_TIMEOUT_   408
#define REQUEST_TOO_LARGE_ 413
#define SERVER_ERR_        500

typedef enum { GET, HEAD } http_method_t;

typedef struct http_request_t_ {
  http_method_t method;
  char path[MAXPATH];
  int httpver;
} http_request_t;

status_t parse_request(char *msg, http_request_t *request);

#endif // __REQUEST_H__
