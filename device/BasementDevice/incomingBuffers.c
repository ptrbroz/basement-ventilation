#include <stdio.h>
#include <stdlib.h>
#include <pico.h>
#include "pico/stdlib.h"
#include "pico/critical_section.h"

#include "incomingBuffers.h"
#include "queue.h"
#include "logging.h"
#include "defines.h"


#define COMMAND_BUFFER_LEN 1024 //Assumption: no AT command line (ranging from <cr><lf> to another <cr><lf>, included) received from SIM7028 will be longer than COMMAND_BUFFER_LEN
#define COMMAND_BUFFER_SLOTS 20 
#define COMMAND_QUEUE_LEN (COMMAND_BUFFER_SLOTS+5) //COMMAND_BUFFER_SLOTS should be enough. Extra so that I don't need to think too hard about corner cases
#define INCOMING_CHAR_QUEUE_LEN 4096

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
            logmsg(debug, "CB %d", activeBuffer);
            if(pushToIntQueue(&fullCommandBuffers, activeBuffer)){
                // queue full -- this should never happen
                logmsg(error, "FCB queue full (wtf!!)");
                commandBuffers[activeBuffer][2] = '\0';     // mark buffer for reuse since it wasn't pushed and as such will not be popped and marked. Probably does not matter
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

/*
bool isrModemUartRx(__unused struct repeating_timer *t){

    int charsRead = 0;

    while(uart_is_readable(MODEM_UART) && (charsRead < maxReadsPerIsr)){
        gpio_put(5, 1);
        logmsg(debug, "start #%d=[%s]", activeBuffer, commandBuffers[activeBuffer] + 2);
        charsRead++;
        char incoming = uart_getc(MODEM_UART);

        commandBuffers[activeBuffer][posInActiveBuffer] = incoming;


        //Detect end of transmission. SIM7028 puts cr lf at start and end of each uart transmission. This ISR assumes that "\r\n" never occurs elsewhere in data.
        if(lastCharWasCR && (incoming == '\n')){
            if(posInActiveBuffer > 1){ //discriminate beginning crlf

                //replace final carriage return with null byte to play nice with string libraries
                commandBuffers[activeBuffer][posInActiveBuffer-1] = '\0';
                commandBufferLen[activeBuffer] = posInActiveBuffer + 1;

                //find a new free buffer, starting at the next one in order
                int nextBuffer = (activeBuffer + 1) % COMMAND_BUFFER_SLOTS;
                while(nextBuffer != activeBuffer){
                    if(commandBuffers[nextBuffer][2] == '\0'){ //check if buffer is free to be overwritten
                        break;
                    }
                    nextBuffer = (nextBuffer + 1) % COMMAND_BUFFER_SLOTS;
                }

                if(nextBuffer == activeBuffer){
                    //failed to find a free buffer
                    logmsg(error, "Could not find a free command buffer! Discarding #%d=[%s].", activeBuffer, commandBuffers[activeBuffer][2]);
                    // don't push buffer index to queue, instead discard its contents and write next command into it.
                    // not ideal -- could get another effective buffer slot for the same memory by only choosing next active buffer upon start of next transmission --
                    // but I don't want to complicate the code (since memory is unlikely to become a concern on the pi pico)
                    commandBuffers[activeBuffer][2] = '\0';
                }
                else{
                    // we were able to find a free buffer to put the next command into
                    // push buffer index to queue
                    if(pushToIntQueue(&fullCommandBuffers, activeBuffer)){
                        logmsg(error, "FCB queue full, failed to push #%d=[%s]!", activeBuffer, commandBuffers[activeBuffer][2]);
                        // this shouldn't ever happen, since max queue size is same as number of buffers. But if it does, let's also mark the current buffer as empty.
                        commandBuffers[activeBuffer][2] = '\0';
                    }
                    else{
                        logmsg(debug, "FCB push B%d=[%s]", activeBuffer, commandBuffers[activeBuffer] + 2); //skip first cr, lf in buffer 
                    }
                }
                activeBuffer = nextBuffer;
                posInActiveBuffer = 0;
            }
            else{
                posInActiveBuffer++; //this 
            }
        }
        else{
            // this is not the end of the transmission
            posInActiveBuffer++;
        }

        lastCharWasCR = (incoming == '\r');
        logmsg(debug, "end #%d=[%s]", activeBuffer, commandBuffers[activeBuffer] + 2);
    }

    if(charsRead>0){
        logmsg(debug,"ISRuart read %d bytes.", charsRead);
    }

    gpio_put(5, 0);
    return true;

}
    */


int tryPopCommand(char **bufferPointer){
    int poppedIndex;
    int retVal = popFromIntQueue(&fullCommandBuffers, &poppedIndex);
    if(retVal!=0){
        *bufferPointer = NULL;
        return 0;
    }
    else{
        // start after first cr, lf, at byte 2. This means that bufferPointer[0] (outside this file) will correspond to commandBuffer[i][2] (inside this file)
        *bufferPointer = commandBuffers[poppedIndex] + 2; 
        return commandBufferLen[poppedIndex] - 3; // stored len includes first crlf which is skipped and last lf which is preceded by string terminator, hence the reduction
    }
}