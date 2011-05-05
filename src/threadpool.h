#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include "datatypes.h"

/* Number of threads used to service client requests */
//#define MIN_WORKER_THREADS 3
#define MAX_WORKER_THREADS 500 /* not sure how to set this effectively */

#define REAPER_TIMEOUT 1000
#define KILL 2
#define IDLE 0
#define BUSY 1
#define DEAD 3

#define CLOSE 0
#define NOTCLOSE 1

// functions
void initRequest(request* req);
void populateRequest(char* requestString, request* newRequest);
void insertRequest(request* newRequest);
request* getNextRequest();
void printRequest(request* req);
void printAllRequests();
void handleRequest(request* req, int threadID);

void* handleLoop(void* data);
void insertWorker();
void deleteWorkers();
void initThreadPool(int initialThreads);

void insertStopID(char* stopID);
void deleteStopID(char* stopID);
int searchStopList(char* stopID);	
void printStopList();
#endif

