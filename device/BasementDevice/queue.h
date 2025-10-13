#if !defined(QUEUE_H)
#define QUEUE_H

typedef struct {
    int *circBuffer;
    int length;
    int head;  // index of next element to be popped
    int count; // number of currently enqued ints
} Queue;

// Allocates memory dynamically! Retval: 0 if succesful, 1 if malloc fail
int initQueue(Queue *q, int length);

// Retval: 0 when success, 1 when queue full
int pushToQueue(Queue *q, int element);

// On retval 0, @popped is populated with an element from queue. Retval 1 indicates that queue was empty and popped is not populated. 
int popFromQueue(Queue *q, int *popped);



#endif // QUEUE_H