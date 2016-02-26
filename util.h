/* System parameters */
#define PORT_    8998
#define BACKLOG_ 10

/* HTTP request methods */
#define GET_  0
#define HEAD_ 1
#define POST_ 2

/* HTTP response status codes */
#define OK_                200
#define BAD_REQUEST_       400
#define FORBIDDEN_         403
#define NOT_FOUND_         404
#define REQUEST_TOO_LARGE_ 413
#define SERVER_ERR_        500

/* content types */
#define HTML_ 0
#define TXT_  1
#define JPG_  2
#define GIF_  3

/* debugging */
#define DEBUG 1
#define DEBUG_PRINT if (DEBUG) printf
