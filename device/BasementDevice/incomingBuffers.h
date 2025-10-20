#if !defined(INCOMINGBUFFERS_H)
#define INCOMINGBUFFERS_H

#include "pico/time.h"


// Allocates memory for command buffers and supporting infrastructure. Only needs to be called once on startup. Returns 0 on success, 1 on fail
int initCommandBuffers();

// ISR for fetching bytes from the modem uart and stuffing them into the buffers.
bool isrModemUartRx(__unused struct repeating_timer *t);

// Get a pointer to a buffer containing next command from modem to be parsed, if there is one waiting.
// BEWARE1: Returns zero on failiure!
// BEWARE2: After you're done with the buffer, mark it as free to be reused by writing \0 at the FIRST position. Ideally copy contents to own buffer.
// NOTE1: To be safe: call this, if you get a result disable interrupts, copy buffer, write \0, enable interrupts, then parse from own buffer.
// retval: length of string in buffer pointed to at bufferpointer, including final \0. If no commands were waiting in queue, retval is 0 and NULL is written to bufferpointer.
int tryPopCommand(char **bufferPointer);







#endif // INCOMINGBUFFERS_H
