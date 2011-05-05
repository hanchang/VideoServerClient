#ifndef BUFFER_H
#define BUFFER_H

// Set maximum slots in the circular buffer.
#define MAXSLOTS 100

// Set m, the number of messages the dispatcher waits for
// before sending a batch of messages back to clients.
// ***WARNING*** MUST BE <= MAXSLOTS!
#define DISPATCHER_BATCH_AMOUNT 20

#include <pthread.h>
#include <ppm.h>
#include "datatypes.h"

// functions
int isBufferFull();
void printBuffer();
void generateMessages(int workerThreadID, request* clientRequest);
void insertToBuffer(int in_workerthread_id, request* clientRequest, int messageID, pixel** in_pixArray, int in_rows, int in_cols, int frame, int repeat);
int compare(struct slot* a, struct slot* b);
void initSlot(int slotNumber);
void initBuffer();
void sendFrame(int bufferIndex);
void process();


#endif
