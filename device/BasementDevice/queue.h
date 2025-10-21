#if !defined(QUEUE_H)
#define QUEUE_H

typedef struct {
    char *circBuffer;
    int length;
    int head;  // index of next element to be popped
    int count; // number of currently enqued ints
} CharQueue;

// Allocates memory dynamically! Retval: 0 if succesful, 1 if malloc fail
int initCharQueue(CharQueue *q, int length);

// Retval: 0 when success, 1 when queue full
int pushToCharQueue(CharQueue *q, char element);

// On retval 0, @popped is populated with an element from queue. Retval 1 indicates that queue was empty and popped is not populated. 
int popFromCharQueue(CharQueue *q, char *popped);

//----------------------------------------------------------------------

typedef struct {
    int *circBuffer;
    int length;
    int head;  // index of next element to be popped
    int count; // number of currently enqued ints
} IntQueue;

// Allocates memory dynamically! Retval: 0 if succesful, 1 if malloc fail
int initIntQueue(IntQueue *q, int length);

// Retval: 0 when success, 1 when queue full
int pushToIntQueue(IntQueue *q, int element);

// On retval 0, @popped is populated with an element from queue. Retval 1 indicates that queue was empty and popped is not populated. 
int popFromIntQueue(IntQueue *q, int *popped);


#endif // QUEUE_H