#if !defined(INCOMINGBUFFERS_H)
#define INCOMINGBUFFERS_H

#include "pico/time.h"


// Allocates memory for command buffers and supporting infrastructure. Only needs to be called once on startup. Returns 0 on success, 1 on fail
int initCommandBuffers();

// Processes chars waiting in incoming queue into command buffers, handling at most maxChars chars. Returns number of chars processed.
// Retval will be lower than maxChars if
int processIncomingCharsIntoBuffers(int maxChars);

// Get a pointer to a buffer containing next command from modem to be parsed, if there is one waiting.
// BEWARE1: Returns zero on failiure!
// BEWARE2: After you're done with the buffer, mark it as free to be reused by writing \0 at index zero. 
// retval: length of string in buffer pointed to at bufferpointer, including final \0. If no commands were waiting in queue, retval is 0 and NULL is written to bufferpointer.
int tryPopCommand(char **bufferPointer);

// ISR for moving chars from 32-byte hw fifo to a larger software queue
void isrMoveCharsToIncomingQueue();





#endif // INCOMINGBUFFERS_H
