
#include "atParser.h"
#include "stateFunctions.h"

typedef struct{
    const char *string;
    ResponseCode code;
} Couple;

const Couple knownCodes[] = {
    {"OK", C_OK},
    {"ERROR", C_ERROR}
};

// loop through the beginning of message, trying to find match in knownCodes
ResponseCode codeParseLoop(char *message){
    int knownCodesLen = sizeof(knownCodes) / sizeof(Couple);
    int impossible[] = {0}; //flags for discriminating nonmatching codes
    int possibleCodesLeft = knownCodesLen;

    int index = 0;
}

void parseAT(StateMachineStruct *stateMachine){



}







