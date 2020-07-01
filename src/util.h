#include <stdio.h>
#include <syslog.h>

#ifndef NDB_LOG_NAME
#ifdef NM_LOG_NAME
#define NDB_LOG_NAME NM_LOG_NAME
#else
#define NDB_LOG_NAME "NickelDBus"
#endif
#endif

#ifndef NDB_VERSION
    #define NDB_VERSION "dev"
#endif

// NDB_LOG writes a log message. Borrowed from NickelMenu
#define NDB_LOG(fmt, ...) syslog(LOG_DEBUG, "(" NDB_LOG_NAME ") " fmt " (%s:%d)", ##__VA_ARGS__, __FILE__, __LINE__)

// Log a message and return 'ret' if cond == fail
#define NDB_ASSERT(ret, cond, fmt, ...) if (!(cond)) { \
    NDB_LOG(fmt, ##__VA_ARGS__); \
    return ret;}
