#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "http.h"

/* A collection of useful functions for parsing
 * HTTP messages. See function parse_request for
 * high-level control flow and work upward.
 */


/* TRY_CATCH and TRY_CATCH_S are private macros that "throw"
 * appropriate status codes whenever a parsing method encounters
 * an error. By wrapping every parsing method call in a TRY_CATCH,
 * errors may be piped up to the original parse_request call.
 * The second TRY_CATCH_S macro is for specially translating
 * the error outputs of the string.h function strsep into the
 * BAD_REQUEST (400) status code.
 */
#define TRY_CATCH(STMT)    do {			      \
                             status_t s = (STMT);     \
			     if (s != OK_) return s;  \
                           } while (0) // wrapped in single-
                                       // iteration loop to
                                       // to allow semicolon
                                       // termination
#define TRY_CATCH_S(STMT)  if ((STMT) == NULL) return BAD_REQUEST_

/* TRY_CATCH_D was for debugging purposes. */
#define TRY_CATCH_D(STMT,METH)    do {			     \
                                    status_t s = (STMT);     \
			            if (s != OK_) { \
	                              printf("error in " METH "\n"); \
			              return s; \
                                    }		\
                                  } while (0)


const char *type_to_str(content_type_t type) {
  switch (type) {
  case APPLICATION: return "application";
  case AUDIO:       return "audio";
  case IMAGE:       return "image";
  case MESSAGE:     return "message";
  case MULTIPART:   return "multipart";
  case TEXT:        return "text";
  case VIDEO:       return "video";
  }
}

const char *status_to_str(status_t status) {
  switch(status) {
  case OK_:                return "OK";
  case BAD_REQUEST_:       return "Bad Request";
  case FORBIDDEN_:         return "Forbidden";
  case NOT_FOUND_:         return "Not Found";
  case REQUEST_TIMEOUT_:   return "Request Timeout";
  case REQUEST_TOO_LARGE_: return "Request Entity Too Large";
  case SERVER_ERR_:
  default:                 return "Internal Server Error";
  }
}


/* Private utility method that acts like strsep(s," \t"), but
 * also advances s so that it skips any additional whitespace.
 */
char *strsep_whitespace(char **s) {
  char *ret = strsep(s," \t");
  while (*s != NULL && (**s == ' ' || **s == '\t'))
    (*s)++; // extra whitespace
  return ret;
}
/* Same as strsep_whitespace, but for newlines. */
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
}

status_t parse_method(char *token, http_request_t *request) {
  if      (strcmp(token,"GET")  == 0) request->method = GET;
  else if (strcmp(token,"HEAD") == 0) request->method = HEAD;
  else {
#if DEBUG
    printf("error in parse_method!\n");
#endif
    return BAD_REQUEST_;
  }

  return OK_;
}

/* Just a simple switch statement for this project. */
status_t parse_path(char *token, http_request_t *request) {
  if (strcmp(token,"/")           == 0 ||
      strcmp(token,"/index.html") == 0 ) {
    snprintf(request->path, MAXPATH, "%s/index.html", DOCUMENT_ROOT);
    request->type = TEXT;
  }
  else if (strcmp(token,COW_FILE) == 0) {
    snprintf(request->path, MAXPATH, "%s" COW_FILE, DOCUMENT_ROOT);
    request->type = IMAGE;
  }
  else if (strcmp(token,SEAL_FILE) == 0) {
    snprintf(request->path, MAXPATH, "%s" SEAL_FILE, DOCUMENT_ROOT);
    request->type = IMAGE;
  }
  else if (strcmp(token,MASCOT_FILE) == 0) {
    snprintf(request->path, MAXPATH, "%s" MASCOT_FILE, DOCUMENT_ROOT);
    //strcpy(request->path, DOCUMENT_ROOT MASCOT_FILE);
    request->type = IMAGE;
  }
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
  //TRY_CATCH_D(  parse_method(token,request), "initial_line 1"  );  
  TRY_CATCH_S(  token = strsep_whitespace(&line)  );
  TRY_CATCH  (  parse_path(token,request)         );
  //TRY_CATCH_D(  parse_path(token,request),  "initial_line 2"   );  
  TRY_CATCH_S(  token = strsep_whitespace(&line)  );
  TRY_CATCH  (  parse_httpver(token,request)      );
  //TRY_CATCH_D(  parse_httpver(token,request), "initial_line 3" );  

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
  TRY_CATCH  (  parse_initial_line(line,request)  );
  //TRY_CATCH_D(  parse_initial_line(line,request), "request init"  );  
  while ( (line = strsep_newline(&msg)) != NULL &&
	  *line != '\0'                      )
    TRY_CATCH(  parse_header(line,request)  );
    //TRY_CATCH_D(  parse_header(line,request), "request header"  );    

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
