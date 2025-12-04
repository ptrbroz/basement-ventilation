#include "stateFunctions.h"
#include "logging.h"
#include "sim7028.h"
#include <string.h>
#include <stdlib.h>



NameListEntry functionNames[] = {
    {begin, "begin"},
    {setupModem, "setupModem"},
    {querySimStatus, "querySimStatus"},
    {querySignalQuality, "querySignalQuality"},
    {queryCSService, "queryCSService"},
    {queryPSService, "queryPSService"},
    {queryUEInfo, "queryUEInfo"}
};
const char errorName[] = "STATE_NAME_NOT_FOUND";

const char * stateFunctionToString(StateFunction *theFun){
    int funCount = sizeof(functionNames) / sizeof(functionNames[0]);
    for(int i = 0; i < funCount; i++){
        if(theFun == functionNames[i].function){
            return functionNames[i].name;
        }
    }
    logmsg(error, "State function name not found in functionNames[]!");
    return errorName;
}

//given a string, find up to @n first occurences of char @c and write their indices into provided char array @positions. Returns number of positions found
int findOccurences(char *string, char c, int n, int *positions){
    int sIndex = 0;
    int rIndex = 0;
    while(rIndex < n){
        char s = string[sIndex];
        if(s == c){
            positions[rIndex] = sIndex;
            rIndex++;
        }
        if(s == '\0'){
            break;
        }
        sIndex++;
    }
    return rIndex - 1;
}


// STATE FUNCTIONS BEGIN

void begin(StateMachineStruct *sm){
    // take some dummy cycles to eat up any unwanted messages that the module spams on startup. 
    // Also, send some ATs -- other libs were doing that, maybe trying to clear modem input?
    if(sm->callCounter<=2){
        sendAT("");
    }
    else if(sm->callCounter>=8){
        sm->nextState = setupModem;
    }
}

void setupModem(StateMachineStruct *sm){
    // TODO: disable urcs here, if needed. If this sends commands, wait for response / add delay before proceeding
    // for now, just proceed further
    sm->nextState = querySimStatus;
}


void querySimStatus(StateMachineStruct *sm){

    if(sm->subState== 0){
        sendAT("+CPIN?");
        sm->subState = 1;
    }
    else if(sm->subState == 1){
        if(sm->code == C_pCPIN){
            char *payload = sm->rawResponse + sm->codeLen;
            int retVal = strcmp(payload, "READY");
            if(retVal == 0){
                logmsg(info, "Sim ready.");
                sm->subState = 2;
            }
            else{
                // not yet ready -> retry
                sm->subState = 0;
            }
        }
    }
    else if(sm->subState == 2){
        //just here to consume the C_OK response
        if(sm->code == C_OK){
            sm->nextState = querySignalQuality;
        }
    }
}

void querySignalQuality(StateMachineStruct *sm){

    if(sm->subState == 0){
        sendAT("+CSQ");
        sm->subState = 1;
    }
    else if(sm->subState == 1){
        if(sm->code == C_pCME_ERROR){
            logmsg(error, "CME ERROR, retrying CSQ [%s]", sm->rawResponse[sm->codeLen]);
            sm->subState = 0;
        }
        else if(sm->code == C_pCSQ){
            char *payload = sm->rawResponse + sm->codeLen;
            char *comma;
            int rssi = strtol(payload,&comma,10);
            int ber = strtol(comma+1, NULL, 10);
            logmsg(info, "CSQ response: rssi=%d, ber=%d", rssi, ber);
            //TODO: do something if signal quality too poor? What?
            sm->subState = 2;
        }
    }
    else if(sm->subState == 2){
        //consume ok
        // TODO CHECK IF ACTUALLY OK SENT
        if(sm->code == C_OK){
            sm->nextState = queryCSService;
        }
    }
}

void queryCSService(StateMachineStruct *sm){

    // this state implements checking "CS domain service" registration that is shown as part of the setup in docs, but probably is not needed
    // in my application. When tried keeps returning 0 (not registered). I think this is for non-data service (calls etc) which my sim might not support
    // Thus, skip.
    bool skipState = true;
    if(skipState){
        logmsg(info, "Skiping CS domain service registration query.");  
        sm->nextState = queryPSService;
        return;
    }

    if(sm->subState == 0){
        sendAT("+CREG?");
        sm->subState = 1;
    }
    else if(sm->subState == 1){
        if(sm->code == C_pCREG){
            char *payload = sm->rawResponse + sm->codeLen;
            int commas[1] = {0};
            int found =  findOccurences(payload, ',', 1, commas);
            int stat = strtol((payload + commas[0] + 1), NULL, 10);
            if(stat == 1 || stat == 5){
                sm->subState = 2;
                logmsg(info, "EPS registered (stat=%d)", stat);
            }
            else{
                // not yet registered -> try again
                sm->subState = 0;
            }
        }
    }
    else if(sm->subState == 2){
        if(sm->code == C_OK){
            sm->nextState = queryPSService;
        }
    }

}

void queryPSService(StateMachineStruct *sm){
    
    if(sm->subState == 0){
        sendAT("+CEREG?");
        sm->subState = 1;
    }
    else if(sm->subState == 1){
        if(sm->code == C_pCEREG){
            char *payload = sm->rawResponse + sm->codeLen;
            int commas[1] = {0};
            findOccurences(payload, ',', 1, commas);

            int stat = strtol((payload + commas[0] + 1), NULL, 10);

            if(stat == 1){
                logmsg(info, "EPS registered (home network)");
                sm->subState = 2;
            }
            else if(stat == 5){
                logmsg(info, "EPS registered (roaming)");
                sm->subState = 2;
            }
            else{
                // not yet registered, try again. Eat OK first.
                sm->subState = 3;
            }
            
        }
    }
    else if(sm->subState == 2){
        // registered, consume OK
        if(sm->code == C_OK){
            sm-> nextState = queryUEInfo;
        }
    }
    else if(sm->subState == 3){
        // not regstered, consume ok and retry
        if(sm->code == C_OK){
            sm->subState = 0;
        }
    }


}

void queryUEInfo(StateMachineStruct *sm){

    if(sm->subState == 0){
        sendAT("+CPSI?");
        sm->subState = 1;
    }
    else if(sm->subState == 1){
        if(sm->code == C_ERROR){
            sm->subState = 0;   // try again I guess?
        }
        else if(sm->code == C_pCPSI){
            logmsg(debug, "enter");
            int commas[2] = {0};
            char *payload = sm->rawResponse + sm->codeLen;
            logmsg(debug, "0[%s]", payload);
            findOccurences(payload, ',', 2, commas);
            logmsg(debug, "foc %d %d", commas[0], commas[1]);
            
            //extract wanted values
            char systemMode[24];
            char operationMode[24];
            int sysLen = commas[0];
            int opLen = commas[1] - (commas[0]+1);
            memcpy(systemMode, payload, sysLen);
            systemMode[sysLen] = '\0';
            memcpy(operationMode, payload + sysLen + 1, opLen);
            operationMode[opLen] = '\0';

            logmsg(debug, "1[%s]", systemMode);
            logmsg(debug, "2[%s]", operationMode);
            
            //check modes
            bool sysmodeOk = (0 == strcmp(systemMode, "NB"));           //might want to add checks for other system modes too? what are the implications?
            bool opmodeOk  = (0 == strcmp(operationMode, "Online"));

            if(sysmodeOk && opmodeOk){
                logmsg(info, "Online in NB mode!");
                logmsg(info, "[%s]", payload);
                sm->subState = 2;
            }
            else{
                sm->subState = 0; //try again
                if(!sysmodeOk){
                    // notify if error is caused by something else than not being signed up yet -- might happen with a different sim card
                    bool noservice = (0 == strcmp(systemMode, "NO SERVICE"));
                    if(!noservice){
                        logmsg(error, "System mode is not NB nor is it NO SERVICE. Are you using the right sim card?");
                        logmsg(error, "System mode is [%s]", systemMode);
                    }
                }
            }
            

       }
    }
    else if(sm->subState == 2){
        if(sm->code == C_OK){
            //sm->nextState = placeholder;
        }
    }

}



void placeholder(StateMachineStruct *sm){

}
