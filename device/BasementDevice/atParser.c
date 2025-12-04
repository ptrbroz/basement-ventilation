
#include "atParser.h"
#include "stateFunctions.h"
#include "incomingBuffers.h"
#include "logging.h"

typedef struct{
    const char *string;
    ResponseCode code;
} Couple;

const Couple knownCodes[] = {
    {"OK", C_OK},
    {"ERROR", C_ERROR},
    {"+CPIN", C_pCPIN},
    {"+CSQ", C_pCSQ},
    {"+CME ERROR", C_pCME_ERROR},
    {"+CREG", C_pCREG},
    {"+CEREG", C_pCEREG},
    {"+CPSI", C_pCPSI}
};

#define knownCodesLen (sizeof(knownCodes) / sizeof(Couple))

// loop through the beginning of message, trying to find match in knownCodes. If succesful, writes length of code to @codeLen
ResponseCode codeParseLoop(char *message, int *codeLen){
    int unmatch[knownCodesLen] = {0}; //flags for discriminating nonmatching codes
    int possibleCodesLeft = knownCodesLen;

    int sIndex = 0;

    //logmsg(debug, "CPL entry kcl = %d", knownCodesLen);

    while(sIndex < COMMAND_BUFFER_LEN){

        char nextChar = message[sIndex];

        // check which codes still match on this char
        for(int i = 0; i < knownCodesLen; i++){
            //logmsg(debug, "check i = %d, unmatch[i] = %d", i, unmatch[i]);
            if(!(unmatch[i])){
                char codeChar = knownCodes[i].string[sIndex];
                //logmsg(debug, "        [%d][%d] %c ~ %c", sIndex, i, nextChar, codeChar);
                if(codeChar == '\0'){
                    // reached end of code string without unmatching char -- this is the code we're looking for
                    *codeLen = sIndex;
                    return knownCodes[i].code;
                }
                else if(codeChar != nextChar){
                    unmatch[i] = 1;
                    possibleCodesLeft--;
                    //logmsg(debug, "UNMATCH");
                }
                else{
                    //logmsg(debug, "match");
                }
            }
        }
        //logmsg(debug, "pcl %d", possibleCodesLeft);

        if(possibleCodesLeft <= 0){
            break;  // all codes unmatched, no reason to continue
        }

        if(nextChar == '\0'){
            break;  // end of incoming message reached
        }

        sIndex++;
    }

    // no code matched
    return C_UNKNOWN;
}

void parseAT(StateMachineStruct *stateMachine){

    if(stateMachine->rawResponse[0] == '\0'){
        stateMachine->code = C_NONE;
        stateMachine->codeLen = 0;
        logmsg(debug, "Parsed C_NONE AT response.");
        return;
    }

    int foundCodeLen;
    ResponseCode retValCode = codeParseLoop(stateMachine->rawResponse, &foundCodeLen);

    if(retValCode == C_UNKNOWN){
        stateMachine->code = C_UNKNOWN;
        stateMachine->codeLen = 0;
        logmsg(debug, "Parsed C_UNKNOWN AT response from msg [%s]", stateMachine->rawResponse);
        return;
    }
    
    // include colons and spaces following the code in codeLen var
    int index = foundCodeLen;
    while(index < COMMAND_BUFFER_LEN){
        char charAtIndex = stateMachine->rawResponse[index];
        if((charAtIndex == ':') || (charAtIndex == ' ')){
            index++;
        }
        else{
            break;
        }
    }

    logmsg(debug, "Parsed code (val %d), from msg [%s]", retValCode, stateMachine->rawResponse);

    stateMachine->code = retValCode;
    stateMachine->codeLen = index;
    return;

}







