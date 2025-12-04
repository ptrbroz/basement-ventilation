#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "defines.h"
#include <string.h>
#include "queue.h"
#include "logging.h"
#include "sim7028.h"
#include "stateFunctions.h"
#include "atParser.h"

absolute_time_t transitionTime;         // timestamp representing time at which last state transition occured


void runFsm(){

    // TODO: some kind of watchdog here that restarts the statemachine if it becomes stuck in a state for too long (to handle, hopefully rare, situation where
    // incoming uart data is corrupted and thus fsm does not move). Could also be handled in individual states (in each one -- do I want to do that?)

    static StateMachineStruct stateMachine = {.nextState = begin};
    static absolute_time_t stateTransitionTime;
    static bool stateTransition = true;
    absolute_time_t currentTime = get_absolute_time();

    if(stateTransition){
        stateTransitionTime = currentTime;
        stateMachine.callCounter = 0;
        stateMachine.subState = 0;
    }

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
        logmsg(debug, "FSM parsing: [%s]", stateMachine.rawResponse);
        parseAT(&stateMachine);
    }

    // calculate time since transition
    stateMachine.msSinceTransition = (absolute_time_diff_us(stateTransitionTime, currentTime)) / 1000;

    // call current state function
    StateFunction *preCallFun = stateMachine.nextState;
    stateMachine.nextState(&stateMachine);
    stateMachine.callCounter++;

    // detect state transition for time update on next run
    stateTransition = (stateMachine.nextState != preCallFun);

    if(stateTransition){
        logmsg(debug, "FSM: %s -> %s. Stats at transition: cc=%d, ss=%d, msst=%d.", stateFunctionToString(preCallFun), stateFunctionToString(stateMachine.nextState), stateMachine.callCounter, stateMachine.subState, stateMachine.msSinceTransition);
    }
    else{
        logmsg(debug, "FSM: @ %s. Stats: cc=%d, ss=%d, msst=%d.", stateFunctionToString(preCallFun), stateMachine.callCounter, stateMachine.subState, stateMachine.msSinceTransition);
    }

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
























