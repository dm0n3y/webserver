#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "http.h"

/* TRY_CATCH and TRY_CATCH_S are private macros that "throw"
 * appropriate status codes whenever a parsing method encounters
 * an error. By wrapping every parsing method call in a TRY_CATCH,
 * errors may be piped up to the original parse_request call.
 * The second TRY_CATCH_S macro is for specially translating
 * the error outputs of the string.h function strsep into the
 * BAD_REQUEST (400) status code.
 */
#define TRY_CATCH_D(STMT,METH)    do {			     \
                             status_t s = (STMT);     \
			     if (s != OK_) { \
	                       printf("error in " METH "\n"); \
			       return s; \
                             }		\
                           } while (0)

#define TRY_CATCH(STMT)    do {			      \
                             status_t s = (STMT);     \
			     if (s != OK_) return s;  \
                           } while (0) // wrapped in single-
                                       // iteration loop to
                                       // to allow semicolon
                                       // termination
#define TRY_CATCH_S(STMT)  do { if ((STMT) == NULL) { printf("error in TRY_CATCH_S\n"); return BAD_REQUEST_; } } while (0)


/* A private utility method that acts like strsep(s," \t"), but
 * also advances s so that it skips any additional whitespace.
 */
char *strsep_whitespace(char **s) {
  char *ret = strsep(s," \t");
  while (*s != NULL && (**s == ' ' || **s == '\t'))
    (*s)++; // extra whitespace
  return ret;
}

char *strsep_newline(char **s) {
  char *r, *n, *ret;
  r = strchr(*s,'\r');
  n = strchr(*s,'\n');
  
  if (r == NULL || n < r)
    ret = strsep(s,"\n");
  else {
    ret = strsep(s,"\r");
    (*s)++; // advance past the trailing \n
  }
  return ret;
  /*
  char *ret = strsep(s,"\r");
  if (*s != NULL && **s == '\n') (*s)++;
  return ret;
  */
}

status_t parse_method(char *token, http_request_t *request) {
  if      (strcmp(token,"GET")  == 0) request->method = GET;
  else if (strcmp(token,"HEAD") == 0) request->method = HEAD;
  else { printf("error in parse_method!\n"); return BAD_REQUEST_; }

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
  else {
    /*
    printf("error in httpver: token is %s\n",token);

    char *c = token;
    while (*c != '\0') {
      printf("%d ",*c);
      c++;
    }
    printf("\n");
    */
    return BAD_REQUEST_;
  }

  return OK_;
}

status_t parse_initial_line(char *line, http_request_t *request) {
  char *token, *ret = line;
  TRY_CATCH_S(  token = strsep_whitespace(&line)  );
  //  TRY_CATCH  (  parse_method(token,request)       );
  TRY_CATCH_D(  parse_method(token,request), "initial_line 1"  );  
  TRY_CATCH_S(  token = strsep_whitespace(&line)  );
  // TRY_CATCH  (  parse_path(token,request)         );
  TRY_CATCH_D(  parse_path(token,request),  "initial_line 2"        );  
  TRY_CATCH_S(  token = strsep_whitespace(&line)  );
  // TRY_CATCH  (  parse_httpver(token,request)      );
  TRY_CATCH_D(  parse_httpver(token,request), "initial_line 3"      );  

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
  TRY_CATCH_S(  line = strsep_newline(&msg)          );
  //TRY_CATCH  (  parse_initial_line(line,request)  );
  TRY_CATCH_D(  parse_initial_line(line,request), "request init"  );  
  while ( (line = strsep_newline(&msg)) != NULL &&
	  *line != '\0'                      )
    //TRY_CATCH(  parse_header(line,request)  );
    TRY_CATCH_D(  parse_header(line,request), "request header"  );    

  return OK_;
}


/* Debugging */
/*
int main() {
  char msg[] = "GET / HTTP/1.0\n\n";
  http_request_t *request = malloc(sizeof(http_request_t));
  printf("\'%s\' returns status %d\n",msg,parse_request(msg,request));
  printf("request->method = %d\nrequest->path = %s\nrequest->httpver = %d\n",request->method, request->path, request->httpver);
  free(request);
}
*/
