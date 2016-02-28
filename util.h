/* System parameters */
extern int PORT;
#define BACKLOG     10
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
#define DEBUG_PRINT if (DEBUG) printf
