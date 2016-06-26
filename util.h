#ifndef __UTIL_H__
#define __UTIL_H__

/* System parameters */
extern int PORT;
#define BACKLOG     1024
#define NUM_WORKERS 25*sysconf(_SC_NPROCESSORS_ONLN)-1
#define MAXMSG      1024

/* File parameters */
extern const char *DOCUMENT_ROOT;
#define COW_FILE      "/Williams-Logo.jpg"
#define SEAL_FILE     "/williams.gif"
#define MASCOT_FILE   "/reading-cow.jpg"
#define MAXPATH       1024

/* Debugging */
#define DEBUG 0
#define DEBUG_QSIZE 0
#define DEBUG_ENQ 0
#define DEBUG_ERR 1

#endif // __UTIL_H__
