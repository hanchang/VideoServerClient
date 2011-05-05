#include "threadpool.h"
#include "buffer.h"
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Reaper stuff
struct timespec reaperTime;
pthread_mutex_t reaperMutex;
pthread_cond_t reaperCondVar;

// Mutex to select which thread will service the next client request
pthread_mutex_t requestMutex = PTHREAD_MUTEX_INITIALIZER;

// Worker threads in the thread pool will block until a pending request arrives
pthread_cond_t  pendingRequest = PTHREAD_COND_INITIALIZER;

// Number of pending requests. 
int numRequests = 0;

// Super-increasing request IDs 
int requestID = 1;

/* Worker thread stuff */
int nextThreadID = 0;
int numWorkerThreads = 0;
int highestThreadID = 0;

// Upper limit of worker threads allowed 
int workerAction[MAX_WORKER_THREADS];

// This is a weird fix... may not need this thing at all 
int workerIDs[MAX_WORKER_THREADS];

// Client requests come in and are placed in a queue (FIFO)
// therefore, we need a head and tail pointer 
// Head of the list of pending requests 
request* requests = NULL;

// Tail of the list of pending requests 
request* tailRequest = NULL;

// Head of the list of clientIDs to stop
stopList* stopHead = NULL;

// Initial worker threads
int MIN_WORKER_THREADS;


// ----
// The stopList is used to keep track of which clientID movie playbacks
// should be stopped (from a stop_movie request).

// Insert a clientID into the stop list. 
void insertStopID(char* stopID) {
	stopList* newStop = (stopList*) malloc(sizeof(stopList));

	// Nice error checking
	if (!newStop) {
		perror("Couldn't allocate memory for insertStopID\n");
		exit(1);
	}

	// Populate our new stopList node
	//newStop->clientID = stopID;
	newStop->clientID = (char*)malloc(sizeof(char) * (strlen(stopID) + 1));
	strcpy(newStop->clientID, stopID);

	// Insert into head
	newStop->next = stopHead;

	// Update head
	stopHead = newStop;					
}


// Remove a clientID from the stopList.  If the requested clientID does
// not exist in the stopList, nothing is done.
void deleteStopID(char* stopID) {
	stopList* current;

	// Start at the head
	current = stopHead;

	if (!current) return;


	// Head is the delete guy
	if (strcmp(current->clientID, stopID) == 0) {
		stopHead = stopHead->next;
		free(current);
		return;
	}

	// Search for the clientID to remove
	while (current->next) {
		if (strcmp(current->next->clientID, stopID) == 0) {
			stopList* deleteGuy = current->next;
			current->next = current->next->next;
			free(deleteGuy);
			return;
		}
	}
}


// Check if the clientID is in the stop list
int searchStopList(char* stopID) {	
	// Start at the head
	stopList* iter = stopHead;

	while (iter) {
		if (strcmp(iter->clientID, stopID) == 0) {
			return TRUE;
		}
		iter = iter->next;
	}

	return FALSE;
}


// Print all clientIDs in the stop list
void printStopList() {
	stopList* iter = stopHead;
	while (iter) {
		printf("clientID: %s\n", iter->clientID);
		iter = iter->next;
	}
}


// Initialize a request object
void initRequest(request* req) {
	req->number = 0;
	req->clientSD = 0;
	req->clientID = 0;
	req->priority = 0;
	req->funcRequest = NULL;
	req->movieName = NULL;
	req->argument = 0;
	req->requestString = NULL;
	req->filePrefix = NULL;
	req->next = NULL;
}


// Insert a request object into the linked list of pending requests
// Worker threads are signaled when a new request is added and compete
// for a mutex.  Requests are serviced FIFO.
void insertRequest(request* newRequest) {
	// Lock the mutex, to assure exclusive access to the list 
	int ret = pthread_mutex_lock(&requestMutex);

	// Add new request to the end of the list, updating list 
	if (numRequests == 0) {
		requests = newRequest;
		tailRequest = newRequest;
	}
	else {
		tailRequest->next = newRequest;
		tailRequest = newRequest;
	}

	// Increase total number of pending requests 
	numRequests++;

	// Check to see if we need more worker threads
	if (numRequests > MIN_WORKER_THREADS) {
		insertWorker();
	}

#ifdef DEBUG
	printf("added new request id: %d\n", newRequest->number);
#endif

	// Unlock mutex 
	ret = pthread_mutex_unlock(&requestMutex);

	// Signal the condition variable - there's a new request to handle 
	ret = pthread_cond_signal(&pendingRequest);
}


// Get next request to be serviced (head)
request* getNextRequest() {
	request* returnRequest;

	if (numRequests > 0) {
		returnRequest = requests;
		requests = returnRequest->next;
		// If this was the last request on the list
		if (requests == NULL) { 
			tailRequest = NULL;
		}
		// decrease the total number of pending requests 
		numRequests--;
	}
	else { 
		returnRequest = NULL;
	}

	return returnRequest;
}


// Print a pending request
void printRequest(request* req) {
	printf("number: %d\n", req->number);
	printf("clientID: %s\n", req->clientID);
	printf("priority: %d\n", req->priority);
	printf("funcRequest: %s\n", req->funcRequest);
	printf("movieName: %s\n", req->movieName);
	printf("argument: %d\n", req->argument);
	printf("request string: %s\n", req->requestString);
}


// Print the list of all pending requests
void printAllRequests() {
	request* iter = requests;
	while (iter) {
		printRequest(iter);
		iter = iter->next;
	}
}


// Eventhandler for a worker thread
void handleRequest(request* req, int threadID) {
	if (req != NULL) {
		// This function is in buffer.c
		generateMessages(threadID, req);
#ifdef DEBUG
		printf("thread: %d handled request %d\n", threadID, req->number);
#endif
	}
}


// Loopback function for a worker thread
// If there are pending requests, and the worker is IDLE, then it attempts
// to grab the mutex and service the next request.  IF there are no pending
// requests then it sleeps by waiting on a condition variable.
// If the thread has been selected for termination (by the reaper thread), then
// it calls pthread_exit() to terminate itself.
void* handleLoop(void* data) {
	int ret;
	request* req = NULL;
	int threadID = *((int*)data);

#ifdef DEBUG
	printf("thread: %d starting thread\n", threadID);
#endif

	// Lock the mutex 
	ret = pthread_mutex_lock(&requestMutex);

	// Perform this loop forever
	while (1) {
		// The thread has been selected for termination by the reaper
		if (workerAction[threadID] == KILL) {
			ret = pthread_mutex_unlock(&requestMutex);
#ifdef DEBUG
			printf("thread: %d about to kill himself\n", threadID);
#endif
			workerAction[threadID] = DEAD;
			pthread_exit(0);
		}
		// There are pending requests and the thread is idle
		else if (workerAction[threadID] == IDLE && numRequests > 0) {
#ifdef DEBUG
			printf("thread: %d about to do some work   numRequests: %d\n", threadID, numRequests);
#endif

			// At this point we should have a mutex lock from wait() or the initial lock
			// So get the next request and service it
			req = getNextRequest();
			if (req != NULL) {
				// Update my status so the reaper won't select me for termination
				workerAction[threadID] = BUSY;

				// Unlock mutex so another thread can get another request 
				ret = pthread_mutex_unlock(&requestMutex);

				handleRequest(req, threadID);
				workerAction[threadID] = IDLE;
				ret = pthread_mutex_lock(&requestMutex);
			}
		}
		else {
			// Wait to be signaled 
#ifdef DEBUG
			printf("thread: %d waiting for signal\n", threadID);
#endif
			ret = pthread_cond_wait(&pendingRequest, &requestMutex);
#ifdef DEBUG
			printf("thread: %d got signaled\n", threadID);
#endif
		}
	}
}


// Insert a new worker thread into thread pool 
void insertWorker() {
	int i, ret;
	pthread_t newWorker;
	workerIDs[nextThreadID] = nextThreadID;

	// Create a new pthread
	ret = pthread_create(&newWorker, NULL, (void *) handleLoop, (void*)&workerIDs[nextThreadID]);
	workerAction[nextThreadID] = IDLE;

	// Find the next threadID to use for the next new worker thread
	for (i = 0; i <= highestThreadID + 1; i++) {
		if (workerAction[i] == DEAD) {
			nextThreadID = i;
			if (nextThreadID > highestThreadID) {
				highestThreadID = nextThreadID;
			}
			numWorkerThreads++;
			break;
		}
	}
}


// Delete worker threads until we are back at the initial number of worker threads
// This is the loopback function for the reaper thread.  The reaper thread uses
// pthread_cond_timedwait to sleep for REAPER_TIMEOUT time.  When it awakes,
// it checks to see if there are idle threads to kill. 
void deleteWorkers() {
	int i, numToDelete;

	while (1) {
		reaperTime.tv_sec = time(NULL) + REAPER_TIMEOUT;
		reaperTime.tv_nsec = 0;
#ifdef DEBUG
		printf("Reaper going to sleep\n");
#endif
		// Sleep for a period of time
		pthread_cond_timedwait(&reaperCondVar, &reaperMutex, &reaperTime);

#ifdef DEBUG
		printf("Reaper about to kill something\n");
#endif

		// Reaper is awake, see if there are idle threads to kill (if number of
		// threads is greater than the initial worker threads
		numToDelete = numWorkerThreads - MIN_WORKER_THREADS;
		i = 0;
		while (numToDelete > 0 && i <= highestThreadID) {
			if (workerAction[i] == IDLE) {
				workerAction[i] = KILL;
				numWorkerThreads--;
				numToDelete--;
			}
			i++;
		}

		// Wake all worker threads so they can check if they should kill themselves
		pthread_cond_broadcast(&pendingRequest);
	}
}


// Initializes the thread pool by creating 'initialThreads' workerthreads
// And creating a dispatcher thread who sleeps initially
// And creating a reaper thread who kills idle threads
void initThreadPool(int initialThreads) {
	int i;

	MIN_WORKER_THREADS = initialThreads;

	// Initialize worker thread data structures
	for (i = 0; i < MAX_WORKER_THREADS; i++) {
		workerAction[i] = DEAD;
		workerIDs[i] = 0;
	}

	// Create initial worker threads
	for (i = 0; i < initialThreads; i++) {
		insertWorker();
	}

	// Initialize dispatcher thread; it will process the m messages.
	pthread_t dispatcher;
	pthread_create(&dispatcher, NULL,(void*) process, NULL);
#ifdef DEBUG
	printf("Dispatcher thread created.\n");
#endif	
	// Create a reaper thread here
	pthread_t reaper;
	pthread_create(&reaper, NULL, (void *) deleteWorkers, NULL);
#ifdef DEBUG
	printf("Reaper thread created.\n");
#endif
}


// Given a client request string populate a new request object
// NOTE! We assume the client is sending valid requests that follow our protocol
void populateRequest(char* requestString, request* newRequest) {
	char* temp = NULL;
	char* delimeter = ":";
	char* copy = NULL;

	// make a copy of the string first
	copy = (char*) malloc(sizeof(char) * (strlen(requestString) + 1));
	strcpy(copy, requestString);

	// Lets be nice and check for stupidity
	if (!newRequest) {
		perror("populateRequest: cannot pass in null newRequest object.\n");
		exit(1);
	}

	// Get the clientID
	temp = strtok(copy, delimeter);
	newRequest->clientID = (char*)malloc(sizeof(char) * (strlen(temp) + 1));
	strcpy(newRequest->clientID, temp);

	// Get the priority
	temp = strtok(NULL, delimeter);
	int priority = atoi(temp);
	newRequest->priority = priority;

	// Get the function request
	temp = strtok(NULL, delimeter);
	newRequest->funcRequest = (char*)malloc(sizeof(char) * (strlen(temp) + 1));
	strcpy(newRequest->funcRequest, temp);

	// Get the movie name
	temp = strtok(NULL, delimeter);
	newRequest->movieName = (char*)malloc(sizeof(char) * (strlen(temp) + 1));
	strcpy(newRequest->movieName, temp);

	// Get the argument (if available)
	temp = strtok(NULL, delimeter);
	if (temp) {
		int argument = atoi(temp);
		newRequest->argument = argument;
#ifdef DEBUG
		printf("argument: %d    newRequest->argument: %d\n", argument, newRequest->argument);
#endif
	}

	// Store a copy of the original request string
	newRequest->requestString = (char*)malloc(sizeof(char) * (strlen(requestString) + 1));
	strcpy(newRequest->requestString, requestString);

	newRequest->next = NULL;
	free(copy);
}


