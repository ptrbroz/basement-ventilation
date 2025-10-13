
#include "incomingBuffers.h"
#include "pico/mutex.h"


#define COMMAND_BUFFER_SLOTS 20
#define COMMAND_BUFFER_LEN 1024 //Assumption: no AT command line (ranging from <cr><lf> to another <cr><lf>) received from SIM7028 will be longer than COMMAND_BUFFER_LEN

char *commandBuffers[COMMAND_BUFFER_SLOTS];             // received AT commands saved for processing
bool commandBufferIsFree[COMMAND_BUFFER_SLOTS];         // can this buffer be overwritten?
mutex_t commandBufferMutex[COMMAND_BUFFER_SLOTS];       // mutexes for buffers to avoid (unlikely) scenario where both the ISR and main loop state machine touch the same buffer

int nextBufferToCheck = 0;                              // next buffer index to look at when searching for a free one
char *activeBuffer = NULL;                              // buffer to which chars are currently being accumulated

