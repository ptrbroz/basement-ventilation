#if !defined(STATEFUNCTIONS_H)
#define STATEFUNCTIONS_H

#include "incomingBuffers.h"

// type for preparsed response codes
typedef enum{
    C_NONE,                   // used when no AT response is present when calling
    C_OK,                     
    C_ERROR,
    C_UNKNOWN                 // if not any of the above, set to unknown -- either something weird to be parsed manually by state function, or an unforeseen URC to be ignored?
} ResponseCode;

typedef struct StateMachineStruct StateMachineStruct;
typedef void (*StateFunction)(StateMachineStruct *);

// struct passed as argument to state functions
struct StateMachineStruct{
    ResponseCode code;                      // at response type, or NONE if state function is being called without new incoming AT response
    char rawResponse[COMMAND_BUFFER_LEN];   // Only relevant if code is not none. The raw received AT message
    int placeholderPayload[100];            // Only relevant if code is not none. Pre-parsed data from incoming response. TODO: figure out how to do this
    int callCounter;                        // counter incremented after each call of state function and reset to 0 on state change
    int msSinceTransition;                  // how many milliseconds have elapsed since last state change
    StateFunction nextState;                // to be modified by StateFunction
};





#endif // STATEFUNCTIONS_H