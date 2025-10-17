#include "logging.h"
#include "stdio.h"
#include "stdarg.h"

LogLevel usbLoggingLevel = debug;

const char logLevelChar[] = "diwE~"; //chars used to prefix log messages of corresponding levels

void UNUSED_logmsg(LogLevel level, const char *format, ...){
    if(level == off){
        return; //off level meant to be reserved for turning off all logging to selected method, so calling this function with it makes no sense
    }
    if(level >= usbLoggingLevel){
        char prefix = logLevelChar[level];
        printf("%c]", prefix);
        char datetimeString[] = "dd.mm.hhmm";     //TODO: get actual datetime once 
        printf("%s:", datetimeString);
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n\r");
    }
}




