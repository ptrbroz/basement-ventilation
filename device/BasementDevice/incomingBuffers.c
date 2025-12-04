#include <stdio.h>
#include <stdlib.h>
#include <pico.h>
#include "pico/stdlib.h"
#include "pico/critical_section.h"

#include "incomingBuffers.h"
#include "queue.h"
#include "logging.h"
#include "defines.h"



// convention: commandBuffer is considered empty (and free to be overwritten) if the THIRD character in it is '\0'
char *commandBuffers[COMMAND_BUFFER_SLOTS];             // received AT commands saved for processing. BEWARE! Final carriage return is replaced with \0
int commandBufferLen[COMMAND_BUFFER_SLOTS];             // length of command stored in buffer, including starting \r\n and final \0\n. 

int activeBuffer = 0;                                  // index of buffer to which chars are currently being accumulated

const int maxReadsPerIsr = 32;                          // only read this many or fewer chars per ISR


IntQueue fullCommandBuffers;                            // indices of command buffers waiting to be parsed and processed
CharQueue incomingChars;                            // queue for storing incoming chars from hardware uart fifo

int initCommandBuffers(){
    static bool alreadyCalled = 0;
    if(alreadyCalled==true){
        logmsg(error, "Command buffs already inited!");
        return 1;
    }
    alreadyCalled = true;

    for(int i=0;i<COMMAND_BUFFER_SLOTS;i++){
        char *newBuffer = malloc(sizeof(char) * COMMAND_BUFFER_LEN);
        if(newBuffer == NULL){
            logmsg(error, "Buffer init fail at malloc #%d", i);
            return 1;
        }
        commandBuffers[i] = newBuffer;
        commandBufferLen[i] = 0;
        newBuffer[2] = '\0';        //mark buffer free to be overwritten
    }

    activeBuffer = 0;

    if(initIntQueue(&fullCommandBuffers, COMMAND_QUEUE_LEN)){    
        logmsg(error, "Buffer init fail to init FCB queue.");
        return 1;
    }

    if(initCharQueue(&incomingChars, INCOMING_CHAR_QUEUE_LEN)){
        logmsg(error, "Buffer init fail to init IC queue.");
        return 1;
    }

    return 0;
}

void isrMoveCharsToIncomingQueue(){
    int charsRead = 0;
    while(uart_is_readable(MODEM_UART) && (charsRead < maxReadsPerIsr)){
        charsRead++;
        char incoming = uart_getc(MODEM_UART);
        if(pushToCharQueue(&incomingChars, incoming)){
            logmsg(error, "ICQ full!");
            break;
        }
    }
}

// updates activeBuffer to be the index of next free buffer. Retval: 0 on success, 1 on fail (all buffers taken)
int updateActiveBuffer(){
    int buffToCheck = (activeBuffer + 1) % COMMAND_BUFFER_SLOTS;
    while(buffToCheck != activeBuffer){
        char marking = commandBuffers[buffToCheck][2];
        if(marking == '\0'){
            // this buffer is marked as free -- claim it
            activeBuffer = buffToCheck;
            return 0;
        }
        else{
            // keep searching
            buffToCheck = (buffToCheck + 1) % COMMAND_BUFFER_SLOTS;
        }
    }
    // ran the full circle -- no buffers are free
    return 1;
}

// ASSUMPTION: <cr><lf> does not occur inside data, so those may be used to detect starts and ends of incoming AT commands
int processIncomingCharsIntoBuffers(int maxChars){

    static bool lastCharWasCR = false;                             // for detecting message edges
    static int posInActiveBuffer = 0;                              // next index to write to in active buff
    static bool activeBufferClosed = false;                        // set to true when a new buffer needs to be found before processing new chars

    int charsProcessed = 0;

    while(charsProcessed < maxChars){

        if(activeBufferClosed){
            int retVal = updateActiveBuffer();
            if(!retVal){ // OK, ready to work on next command
                activeBufferClosed = false;
                posInActiveBuffer = 0;
                lastCharWasCR = false;
            }
            else{
                // failed to find a free buffer -- don't pop any more chars as there will be nowhere to put them
                break;
            }
        }

        char poppedChar;

        // incomingChars queue is accessed by uart ISR -- thus the disable
        uint32_t irqStatus = save_and_disable_interrupts();
        int retVal = popFromCharQueue(&incomingChars, &poppedChar);
        restore_interrupts_from_disabled(irqStatus);

        if(retVal){
            // incoming char queue was empty -- no more work to be done
            break;
        }

        charsProcessed++;
        commandBuffers[activeBuffer][posInActiveBuffer] = poppedChar;

        if(lastCharWasCR&&(poppedChar=='\n')&&(posInActiveBuffer>1)){
            // end of command detected -- time to close the buffer
            commandBuffers[activeBuffer][posInActiveBuffer - 1] = '\0'; //replace last CR with terminator to play nice with str funs
            commandBufferLen[activeBuffer] = posInActiveBuffer+1;
            activeBufferClosed = true;

            if(commandBufferLen[activeBuffer] <= 3){
                // corner case with weird input -- len of 3 means first two bytes will be skipped (as they are assumed, wrongly, to be crlf) and the third byte
                // will be the terminator. This will also cause tryPop to return zero or lower len... generally, lets not even push this to the queue and just discard it.
                logmsg(debug, "CB#%d weird input (too short, CBL=%d) -- discarding [%s]", activeBuffer, commandBufferLen[activeBuffer], commandBuffers[activeBuffer]);
                commandBuffers[activeBuffer][2] = '\0'; // mark for reuse -- not currently necessary as corner case likely only occurs for CBL=3, just to be sure
            }
            else{
                logmsg(debug, "push CB#%d : [%s]", activeBuffer, commandBuffers[activeBuffer]+2);
                if(pushToIntQueue(&fullCommandBuffers, activeBuffer)){
                    // queue full -- this should never happen
                    logmsg(error, "FCB queue full (wtf!!)");
                    commandBuffers[activeBuffer][2] = '\0';     // mark buffer for reuse since it wasn't pushed and as such will not be popped and marked. Probably does not matter
                }
            }
        }
        else{
            // this is not yet the end
            posInActiveBuffer++;
            lastCharWasCR = (poppedChar == '\r');
        }
    }

    return charsProcessed;

}

int tryPopCommand(char **bufferPointer){

    uint32_t irqStatus = save_and_disable_interrupts(); 
    //TODO: consider whether interrupts have to be disabled here. Disabling for now to be safe (what happens when popping from queue is interrupted by ISR that pushes to it?)

    int poppedIndex;
    int retVal = popFromIntQueue(&fullCommandBuffers, &poppedIndex);

    restore_interrupts_from_disabled(irqStatus);

    if(retVal!=0){
        *bufferPointer = NULL;
        return 0;
    }
    else{
        logmsg(debug, "pop CB %d, len %d", poppedIndex, commandBufferLen[poppedIndex] - 3);
        // start after first cr, lf, at byte 2. This means that bufferPointer[0] (outside this file) will correspond to commandBuffer[i][2] (inside this file)
        *bufferPointer = commandBuffers[poppedIndex] + 2; 
        return commandBufferLen[poppedIndex] - 3; // stored len includes first crlf which is skipped and last lf which is preceded by string terminator, hence the reduction
    }
}

void discardBuffers(){
    while(1){
        int bufNum;
        int retVal = popFromIntQueue(&fullCommandBuffers, &bufNum);
        if(retVal){
            break;      // queue empty
        }
        else{
            // set popped for reuse
            commandBuffers[bufNum][2] = '\0';
        }
    }
}