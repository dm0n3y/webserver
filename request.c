#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "request.h"

/* TRY_CATCH and TRY_CATCH_S are private macros that "throw"
 * appropriate status codes whenever a parsing method encounters
 * an error. By wrapping every parsing method call in a TRY_CATCH,
 * errors may be piped up to the original parse_request call.
 * The second TRY_CATCH_S macro is for specially translating
 * the error outputs of the string.h function strsep into the
 * BAD_REQUEST (400) status code.
 */
#define TRY_CATCH(STMT)    do {                       \
                             status_t s = (STMT);     \
			     if (s != OK_) return s;  \
                           } while (0) // wrapped in single-
                                       // iteration loop to
                                       // to allow semicolon
                                       // termination
#define TRY_CATCH_S(STMT)  if ((STMT) == NULL) return BAD_REQUEST_


/* A private utility method that acts like strsep(s," \t"), but
 * also advances s so that it skips any additional whitespace.
 */
char *strsep_whitespace(char **s) {
  char *ret = strsep(s," \t");
  while (*s != NULL && (**s == ' ' || **s == '\t'))
    (*s)++; // extra whitespace
  return ret;
}

status_t parse_method(char *token, http_request_t *request) {
  if      (strcmp(token,"GET")  == 0) request->method = GET_;
  else if (strcmp(token,"HEAD") == 0) request->method = HEAD_;
  else if (strcmp(token,"POST") == 0) request->method = POST_;
  else return BAD_REQUEST_;

  return OK_;
}

/* Just a simple switch statement for this assignment. */
status_t parse_path(char *token, http_request_t *request) {
  if (strcmp(token,"/")           == 0 ||
      strcmp(token,"/index.html") == 0 )
    strcpy(request->path, DOCUMENT_ROOT "/index.html");
  else if (strcmp(token,COW_FILE) == 0)
    strcpy(request->path, DOCUMENT_ROOT COW_FILE);
  else if (strcmp(token,SEAL_FILE) == 0)
    strcpy(request->path, DOCUMENT_ROOT SEAL_FILE);
  else if (strcmp(token,MASCOT_FILE) == 0)
    strcpy(request->path, DOCUMENT_ROOT MASCOT_FILE);
  else
    return NOT_FOUND_;
  
  return OK_;
}

status_t parse_httpver(char *token, http_request_t *request) {
  if      (strcmp(token,"HTTP/1.0") == 0) request->httpver = 0;
  else if (strcmp(token,"HTTP/1.1") == 0) request->httpver = 1;
  else return BAD_REQUEST_;

  return OK_;
}

status_t parse_initial_line(char *line, http_request_t *request) {
  char *token, *ret = line;
  TRY_CATCH_S(  token = strsep_whitespace(&line)  );
  TRY_CATCH  (  parse_method(token,request)       );
  TRY_CATCH_S(  token = strsep_whitespace(&line)  );
  TRY_CATCH  (  parse_path(token,request)         );
  TRY_CATCH_S(  token = strsep_whitespace(&line)  );
  TRY_CATCH  (  parse_httpver(token,request)      );

  return OK_;
}

/* Currently ignores any request headers, as I assume that
 * any incoming requests have no content beyond headers.
 */
status_t parse_header(char *line, http_request_t *request) {
  return OK_;
}

status_t parse_request(char *msg, http_request_t *request) {
  char *line, *ret = msg;
  TRY_CATCH_S(  line = strsep(&msg,"\n")          );
  TRY_CATCH  (  parse_initial_line(line,request)  );
  while ( (line = strsep(&msg,"\n")) != NULL &&
	  *line != '\0'                      )
    TRY_CATCH(  parse_header(line,request)  );

  return OK_;
}


/* Debugging */
int main() {
  char msg[] = "POST /Williams-Logo.jpg HTTP/1.1";
  http_request_t *request = malloc(sizeof(http_request_t));
  printf("\'%s\' returns status %d\n",msg,parse_request(msg,request));
  printf("request->method = %d\nrequest->path = %s\nrequest->httpver = %d\n",request->method, request->path, request->httpver);
  free(request);
}
