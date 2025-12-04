#if !defined(STATEFUNCTIONS_H)
#define STATEFUNCTIONS_H

#include "incomingBuffers.h"

// type for preparsed response codes
typedef enum{
    C_NONE,                   // used when no AT response is present when calling
    C_OK,                     
    C_ERROR,
    C_pCPIN,
    C_pCSQ,
    C_pCME_ERROR,
    C_pCREG,
    C_pCEREG,
    C_pCPSI,
    C_UNKNOWN                 // if not any of the above, set to unknown -- either something weird to be parsed manually by state function, or an unforeseen URC to be ignored?
} ResponseCode;

typedef struct StateMachineStruct StateMachineStruct;
typedef void (StateFunction)(StateMachineStruct *);

// returns a pointer to string holding state name
const char * stateFunctionToString(StateFunction *);

// struct passed as argument to state functions
struct StateMachineStruct{
    int callCounter;                        // counter incremented after each call of state function and reset to 0 on state change
    int msSinceTransition;                  // how many milliseconds have elapsed since last state change
    int subState;                           // to be modified by StateFunction, for tracking multi-step procedures within more complicated states. Zeroed on state transition
    StateFunction *nextState;               // to be modified by StateFunction


    ResponseCode code;                      // AT response type, or C_NONE if state function is being called without new incoming AT response
    // Following members are only relevant if code is not C_NONE. If code is C_NONE, they may hold leftovers from previous code.
    char rawResponse[COMMAND_BUFFER_LEN];   // The raw received AT message.
    int codeLen;                            // If code is not C_UNKNOWN, this holds the number of bytes taken by response code, plus any colons and spaces directly following. Thus, response payload (if any) will start at rawResponse[codeLen].
};

StateFunction begin;
StateFunction setupModem;
StateFunction querySimStatus;
StateFunction querySignalQuality;
StateFunction queryCSService;
StateFunction queryPSService;
StateFunction queryUEInfo;
StateFunction setupPDPContext;

typedef struct{
    StateFunction *function;
    const char *name;
} NameListEntry;

extern NameListEntry functionNames[];

#endif // STATEFUNCTIONS_H