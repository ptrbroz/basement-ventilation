#include "queue.h"
#include "stdlib.h"


int initCharQueue(CharQueue *q, int length){
    q->circBuffer = malloc(sizeof(char) * length);
    if(q->circBuffer == NULL){ //malloc failed.
        return 1;   
    }
    q->length = length;
    q->head = 0;
    q->count = 0;
    return 0;
}

int pushToCharQueue(CharQueue *q, char element){
    if(q->count == q->length){
        return 1;
    }
    int index = (q->head + q->count) % q->length;
    q->circBuffer[index] = element;
    q->count++;
    return 0;
}

int popFromCharQueue(CharQueue *q, char *popped){
    if(q->count == 0){
        return 1;
    }
    *popped = q->circBuffer[q->head];
    q->head = (q->head + 1) % q->length;
    q->count--;
    return 0;
}

//---------------------------------------------------------

int initIntQueue(IntQueue *q, int length){
    q->circBuffer = malloc(sizeof(int) * length);
    if(q->circBuffer == NULL){ //malloc failed.
        return 1;   
    }
    q->length = length;
    q->head = 0;
    q->count = 0;
    return 0;
}

int pushToIntQueue(IntQueue *q, int element){
    if(q->count == q->length){
        return 1;
    }
    int index = (q->head + q->count) % q->length;
    q->circBuffer[index] = element;
    q->count++;
    return 0;
}

int popFromIntQueue(IntQueue *q, int *popped){
    if(q->count == 0){
        return 1;
    }
    *popped = q->circBuffer[q->head];
    q->head = (q->head + 1) % q->length;
    q->count--;
    return 0;
}
