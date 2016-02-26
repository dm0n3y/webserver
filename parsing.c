#include <stdlib.h>
#include <stdio.h>

#include "util.h"

#define TRY_CATCH_S(STMT)  if ((STMT) == NULL) return BAD_REQUEST_
#define TRY_CATCH  (STMT)  do {                         \
                             http_status_t s = (STMT);  \
			     if (s != 0) return s;      \
                           } while (0)
#define SKIP_SPACE(c)      while (*c == ' ' || *c == '\t') c++

#define MAXPATH 1024

typedef int http_status_t;

typedef enum { INDEX, COW, SEAL, MASCOT } resource_t;

typedef struct http_request_t_ {
  int method;
  char filepath[MAXPATH];
  int httpver;
} http_request_t;


char *parse_request(char *msg, http_request_t *request) {
  char *line, *ret = msg;
  TRY_CATCH(  line = strsep(&msg,"\n")  );
  TRY_CATCH(  parse_initial_line(line,request)  );
  while ( (line = strsep(&msg,"\n")) != NULL && *line != '\0' )
    TRY_CATCH(  parse_header(line,request)  );
  return ret;
}

char *parse_initial_line(char *line, http_request_t *request) {
  char *token, *ret = line;
  TRY_CATCH(  token = parse_whitespace(&line)  );
  TRY_CATCH(  parse_method(token,request)      );
  TRY_CATCH(  token = parse_whitespace(&line)  );
  TRY_CATCH(  parse_filepath(token,request)    );
  TRY_CATCH(  token = prase_whitespace(&line)  );
  TRY_CATCH(  parse_httpver(token,request)     );
  return ret;
}

char *parse_whitespace(char **s) {
  char *ret;
  TRY_CATCH(  ret = strsep(s," \t")  );
  while (**s == ' ' || **s == '\t') (*s)++;
  return ret;
}

char *parse_method(char *token, http_request_t *request) {
  if      (strcmp(token,"GET")  == 0) request->method = GET_;
  else if (strcmp(token,"HEAD") == 0) request->method = HEAD_;
  else if (strcmp(token,"POST") == 0) request->method = POST_;
  else return NULL;
}

char *parse_filepath(char *token, http_request_t *request) {
  char *dir, *ret = token;
  TRY_CATCH(  dir = strsep(&token,"/")  );
}

