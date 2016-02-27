/* System parameters */
#define PORT_       8999
#define BACKLOG_    10
#define NUM_WORKERS 25*sysconf(_SC_NPROCESSORS_ONLN)-1
#define MAXMSG      1024

/* File parameters */
// CHANGE THIS WHEN SWITCHING TO UNIX MACHINE
#define MAXPATH       1024
//#define DOCUMENT_ROOT "/Network/Servers/fuji.cs.williams.edu/Volumes/Users3/15dm7/Documents/courses/cs339/a1/resources"
#define DOCUMENT_ROOT "/home/cs-students/15dm7/cs339/a1/resources"
#define COW_FILE      "/Williams-Logo.jpg"
#define SEAL_FILE     "/williams.gif"
#define MASCOT_FILE   "/reading-cow.jpg"

/* content types */
#define HTML 0
#define TXT  1
#define JPG  2
#define GIF  3

/* debugging */
#define DEBUG 1
#define DEBUG_PRINT if (DEBUG) printf
