#include <iostream>
#include <string>
#include "util.h"
using namespace std;

class HTTPRequest {

  /* initial request line */
  int method;
  string path;
  int version; // 0 or 1

};

/*
HTTPRequestHandler
HTTPResponseHandler


control flow:
- receive and parse request into some data structure
  - if too large, then need to send 413
  - if poorly formed, then need to send 400
- carry out request, e.g., look up file
  - if file not found, send 404
  - 

- all the while, if there is error on server side, send 500

- okay, so how do I flow through this exactly?
- do I build up a response as I go?
- feels like a goto statement would be nice here...
  - in more structured flow, I guess it's really a sequence of nested if statements..
- okay, define HTTPResponseHandler, call appropriate method when error encountered,
  then return from overall control flow, since nothing after that matters


current restrictions/assumptions:
- use C!
- HTTP/1.0
- assuming client is always sends the initial line 'GET / HTTP/1.0'
  and so server will always succeed
- server does not send any content
- don't worry about performance

*/
