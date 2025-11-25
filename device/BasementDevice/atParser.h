#if !defined(ATPARSER_H)
#define ATPARSER_H

#include "stateFunctions.h"

// Attempts to parse AT response in @stateMachine->rawResponse and fill out code and possibly other relevant fields of the struct
void parseAT(StateMachineStruct *stateMachine);

#endif // ATPARSER_H