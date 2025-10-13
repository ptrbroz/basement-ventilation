#include "queue.h"
#include "stdlib.h"


int initQueue(Queue *q, int length){
    q->circBuffer = malloc(sizeof(int) * length);
    if(q->circBuffer == NULL){ //malloc failed.
        return 1;   
    }
    q->length = length;
    q->head = 0;
    q->count = 0;
    return 0;
}

int pushToQueue(Queue *q, int element){
    if(q->count == q->length){
        return 1;
    }
    int index = (q->head + q->count) % q->length;
    q->circBuffer[index] = element;
    q->count++;
    return 0;
}

int popFromQueue(Queue *q, int *popped){
    if(q->count == 0){
        return 1;
    }
    *popped = q->circBuffer[q->head];
    q->head = (q->head + 1) % q->length;
    q->count--;
    return 0;
}


