#include <stdio.h>
#include "pico/stdlib.h"
#include "defines.h"
#include <string.h>
#include "queue.h"


void sendAT(char *str){
    const int basicLen = 7; //2x2 for crlfs, 1x2 for AT, 1 for terminator
    int totalLen = basicLen + strlen(str);
    char strToSend[totalLen];
    __asm__ __volatile__ ("" ::: "memory");

    strToSend[0] = '\r';
    strToSend[1] = '\n';
    strToSend[2] = 'A';
    strToSend[3] = 'T';
    strcpy((strToSend+4), str);
    strToSend[totalLen-3] = '\r'; //has to be done after strcpy due to null terminator in there
    strToSend[totalLen-2] = '\n'; 
    strToSend[totalLen-1] = '\0'; //not needed, see below, leaving to make future-fool proof

    for(int i = 0; i < (totalLen-1); i++){
        uart_putc_raw(MODEM_UART, strToSend[i]); // uart_puts threatens to do some crlf conversions. Let's not risk that.
    }
}


