#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "defines.h"
#include <string.h>
#include "queue.h"
#include "logging.h"
#include "sim7028.h"
#include "stateFunctions.h"

absolute_time_t transitionTime;         // timestamp representing time at which last state transition occured


void runFsm(){

    static StateMachineStruct stateMachine;

    // check for incoming AT command
    char *incomingBuffer;
    int retLen = tryPopCommand(&incomingBuffer);

    if(retLen == 0){
        // no incoming command was waiting
        stateMachine.code = C_NONE;
    }
    else{
        memcpy( &(stateMachine.rawResponse), incomingBuffer, retLen);
        incomingBuffer[0] = '\0'; // mark buffer free for reuse

    }

    

    // calculate time since transition

    // call current state function (arguments: callCount, time since transition, incoming AT struct. Retval: next state function)

    // if state function differs, update transitionTime

}






void sendAT(char *str){
    const int basicLen = 7;         // 2x2 for crlfs, 1x2 for AT, 1 for terminator
    const int maxMsgLen = 550;      // sim7028 docs say maximum 559, bit lower to be sure (idk if they count the last crlf)
    const int totalBufLen = basicLen + maxMsgLen;    
    char strToSend[totalBufLen]; 

    unsigned int inputLen = strlen(str);
    if(inputLen > maxMsgLen){
        logmsg(error, "AT command too long (%d chars), cannot send!", inputLen);
        return;
    }
    int totalMsgLen = basicLen + inputLen;

    strToSend[0] = '\r';
    strToSend[1] = '\n';
    strToSend[2] = 'A';
    strToSend[3] = 'T';
    memcpy((strToSend+4), str, inputLen);
    strToSend[totalMsgLen-3] = '\r'; //has to be done after strcpy due to null terminator in there
    strToSend[totalMsgLen-2] = '\n'; 
    strToSend[totalMsgLen-1] = '\0'; //not needed, see below, leaving to make future-fool proof

    for(int i = 0; i < (totalMsgLen-1); i++){
        uart_putc_raw(MODEM_UART, strToSend[i]); // uart_puts threatens to do some crlf conversions. Let's not risk that.
    }
    logmsg(debug, "Sent AT[%s]", str);
}
























