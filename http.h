#ifndef __HTTP_H__
#define __HTTP_H__

typedef int status_t;
#define OK_                200
#define BAD_REQUEST_       400
#define FORBIDDEN_         403
#define NOT_FOUND_         404
#define REQUEST_TIMEOUT_   408
#define REQUEST_TOO_LARGE_ 413
#define SERVER_ERR_        500
const char *status_to_str(status_t s);

typedef enum { GET, HEAD } http_method_t;

typedef enum { 
  APPLICATION,
  AUDIO,
  IMAGE,
  MESSAGE,
  MULTIPART,
  TEXT,
  VIDEO
} content_type_t;

const char *type_to_str(content_type_t type);

typedef struct http_request_t_ {
  http_method_t method;
  char path[MAXPATH];
  content_type_t type;
  int httpver;
} http_request_t;


status_t parse_request(char *msg, http_request_t *request);

#endif // __HTTP_H__
