#ifndef DATATYPES_H
#define DATATYPES_H

#include <ppm.h>

/* Constants */
#define TRUE 1
#define FALSE 0

//#define DEBUG
 
typedef struct request_t {
   int number;
   int clientSD;
   char* clientID;
   int priority;
   char* funcRequest;
   char* movieName;
   int argument;
	char* requestString;
	char* filePrefix;
   struct request_t* next;
} request;

// Struct holding all the information required in a buffer slot.
struct slot {
   int isFilled;
   int workerthread_id;
   int priority; // hack, duplicate of clientRequest->priority but need it for qsort
	int messageID;
	pixel** pixArray;
	int rows;
	int cols;
	int frame;
	int repeat;
   request* clientRequest;
};

typedef struct stopList_t {
	char* clientID;
	struct stopList_t* next;
} stopList;


#endif
