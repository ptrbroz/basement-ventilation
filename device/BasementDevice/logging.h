#if !defined(LOGGING_H)
#define LOGGING_H

#include <stdarg.h>

typedef enum{
    debug = 0,
    info = 1,
    warn = 2,
    error = 3,
    off = 4
}LogLevel;


// logging method level variables
extern LogLevel usbLoggingLevel;
// other logging methods will go here, if implemented (log to flash, log over mqtt?)

// log message, using printf() format, to logging methods according to logging method level variables in logging.h.
// A logging method will only receive the log if @level is greater than or equal to value of the corresponding variable.
// If called with @level set to off, no log will be produced to any method.
// \n\r is added to end of logged line automatically and thus shouldn't be included in format
inline void __attribute__((always_inline)) logmsg(LogLevel level, const char *format, ...);


#endif // LOGGING_H

