#include <stdio.h>
#include <stdlib.h>
#include <pico.h>
#include "pico/stdlib.h"

#include "incomingBuffers.h"
//#include "pico/critical_section.h"
#include "queue.h"
#include "logging.h"
#include "defines.h"


#define COMMAND_BUFFER_SLOTS 20 
#define COMMAND_BUFFER_LEN 1024 //Assumption: no AT command line (ranging from <cr><lf> to another <cr><lf>, included) received from SIM7028 will be longer than COMMAND_BUFFER_LEN

// convention: commandBuffer is considered empty (and free to be overwritten) if the THIRD character in it is '\0'
char *commandBuffers[COMMAND_BUFFER_SLOTS];             // received AT commands saved for processing. BEWARE! Final carriage return is replaced with \0
int commandBufferLen[COMMAND_BUFFER_SLOTS];             // length of command stored in buffer, including starting \r\n and final \0\n. 

int activeBuffer = 0;                                   // index of buffer to which chars are currently being accumulated
int posInActiveBuffer = 0;                              // next index to write to in active buff

const int maxReadsPerIsr = 32;                          // only read this many or fewer chars per ISR

bool lastCharWasCR = false;                             //for detecting message edges

Queue fullCommandBuffers;

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
    posInActiveBuffer = 0;
    lastCharWasCR = false;

    if(initQueue(&fullCommandBuffers, COMMAND_BUFFER_SLOTS)){
        logmsg(error, "Buffer init fail to init queue.");
        return 1;
    }

    return 0;
}

bool isrModemUartRx(__unused struct repeating_timer *t){

    //logmsg(debug, "ISRuart IN");

    int charsRead = 0;

    while(uart_is_readable(MODEM_UART) && (charsRead < maxReadsPerIsr)){
        gpio_put(5, 1);
        logmsg(debug, "start #%d=[%s]", activeBuffer, commandBuffers[activeBuffer] + 2);
        charsRead++;
        char incoming = uart_getc(MODEM_UART);

        commandBuffers[activeBuffer][posInActiveBuffer] = incoming;

        /*
        if(incoming == '\n'){
            logmsg(debug, "R NL");
        }
        else if (incoming == '\r'){
            logmsg(debug, "R CR");
        }
        else{
            logmsg(debug, "R %c", incoming);
        }
        */


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
                    if(pushToQueue(&fullCommandBuffers, activeBuffer)){
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

int tryPopCommand(char **bufferPointer){
    int poppedIndex;
    int retVal = popFromQueue(&fullCommandBuffers, &poppedIndex);
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
