#include "logging.h"
#include "stdio.h"
#include "stdarg.h"

LogLevel usbLoggingLevel = info;

const char logLevelChar[] = "diwE~"; //chars used to prefix log messages of corresponding levels

void logmsg(LogLevel level, const char *format, ...){
    if((level) == off){                                     
        return;                                              
    }                                                       
    if((level) >= usbLoggingLevel){                         
        char prefix = logLevelChar[(level)];                
        char datetimeString[] = "dd.mm.hhmm"; /*TODO time*/ 
        printf("%c]%s:", prefix, datetimeString);           
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n\r");                                     
    }                                                       
}





