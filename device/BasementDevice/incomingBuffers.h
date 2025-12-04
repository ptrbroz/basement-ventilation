#if !defined(INCOMINGBUFFERS_H)
#define INCOMINGBUFFERS_H

#include "pico/time.h"

#define COMMAND_BUFFER_LEN 1024 //Assumption: no AT command line (ranging from <cr><lf> to another <cr><lf>, included) received from SIM7028 will be longer than COMMAND_BUFFER_LEN
#define COMMAND_BUFFER_SLOTS 20 
#define COMMAND_QUEUE_LEN (COMMAND_BUFFER_SLOTS+5) //COMMAND_BUFFER_SLOTS should be enough. Extra so that I don't need to think too hard about corner cases
#define INCOMING_CHAR_QUEUE_LEN 4096

// Allocates memory for command buffers and supporting infrastructure. Only needs to be called once on startup. Returns 0 on success, 1 on fail
int initCommandBuffers();

// Processes chars waiting in incoming queue into command buffers, handling at most maxChars chars. Returns number of chars processed.
// Retval will be lower than maxChars if there were fewer chars in queue or if we run out of free command buffers.
int processIncomingCharsIntoBuffers(int maxChars);

// Get a pointer to a buffer containing next command from modem to be parsed, if there is one waiting.
// BEWARE1: Returns zero on failiure!
// BEWARE2: After you're done with the buffer, mark it as free to be reused by writing \0 at index zero. 
// retval: length of string in buffer pointed to at bufferpointer, including final \0. If no commands were waiting in queue, retval is 0 and NULL is written to bufferpointer.
int tryPopCommand(char **bufferPointer);

// ISR for moving chars from 32-byte hw fifo to a larger software queue
void isrMoveCharsToIncomingQueue();

// Removes all currently stored commands from queue and marks buffers for reuse.
void discardBuffers();





#endif // INCOMINGBUFFERS_H
