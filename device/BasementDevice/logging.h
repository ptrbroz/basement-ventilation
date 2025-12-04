#if !defined(LOGGING_H)
#define LOGGING_H

#include "stdio.h"
#include "stdarg.h"

typedef enum{
    debug = 0,
    info = 1,
    warn = 2,
    error = 3,
    off = 4
}LogLevel;

extern const char logLevelChar[];

// logging method level variables
extern LogLevel usbLoggingLevel;
// other logging methods will go here, if implemented (log to flash, log over mqtt?)

// log message, using printf() format, to logging methods according to logging method level variables in logging.h.
// A logging method will only receive the log if @level is greater than or equal to value of the corresponding variable.
// If called with @level set to off, no log will be produced to any method.
// \n\r is added to end of logged line automatically and thus shouldn't be included in format
void UNUSED_logmsg(LogLevel level, const char *format, ...);

void test_logmsg(LogLevel level, const char *format, ...);

// Same as UNUSED_logmsg, but rewritten as macro to avoid overhead. Turns out "function 'logmsg' can never be inlined because it uses variable argument lists"...
#define logmsg(level, format, ...)                              \
    do{                                                         \
        if((level) == off){                                     \
            break;                                              \
        }                                                       \
        if((level) >= usbLoggingLevel){                         \
            char prefix = logLevelChar[(level)];                \
            char datetimeString[] = "dd.mm.hhmm"; /*TODO time*/ \
            printf("%c]%s:", prefix, datetimeString);           \
            printf((format), ##__VA_ARGS__);                    \
            printf("\n\r");                                     \
        }                                                       \
    } while(0)


#endif // LOGGING_H

