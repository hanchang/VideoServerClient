#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ppm.h>
#include <signal.h>

#include "buffer.h"
#include "threadpool.h"


extern int errno;

// Mutex to guard access to circular buffer.
pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;

// Condition variable that the dispatcher thread looking
// in the circular buffer will block against.
pthread_cond_t bufferWait = PTHREAD_COND_INITIALIZER;

// Getting deadlocks when using bufferWait for both workers and dispatcher
// Dispatcher was suffering from starvation, so he has his own cond var now
pthread_cond_t dispatcherWait = PTHREAD_COND_INITIALIZER;

// Circular buffer where messages are placed 
struct slot buffer[MAXSLOTS];

// Set counter variable for number of open slots.
int openSlots = MAXSLOTS;


// Returns 1 if the buffer is full, 0 otherwise.
	int isBufferFull() {
		if (openSlots == 0)
			return TRUE;
		else
			return FALSE;
	}


// Prints the contents of the buffer.
void printBuffer() {
	int i = 0;
	printf("Printing buffer contents: \n");
	for(i = 0; i < MAXSLOTS; i++) {
		if (buffer[i].isFilled == TRUE) {
			printf("Slot %d: Message id: %d   Priority: %d,  ClientID: %s\n",
					i,
					buffer[i].messageID,
					buffer[i].clientRequest->priority,
					buffer[i].clientRequest->clientID);
		}
	}
}


// Inserts a worker thread's text message into the next
// available slot in the circular buffer.
void insertToBuffer(int in_workerthread_id, request* clientRequest, int messageID, 
		pixel** in_pixArray, int in_rows, int in_cols, int in_frame, int in_repeat) {
	while (1) {
		// Check for empty slots; if no empty slots, wait.
		if (isBufferFull() == TRUE) {
			pthread_cond_wait(&bufferWait, &bufferMutex);
		}
		else {
			// Grab buffer mutex to ensure exclusive access to buffer.
			pthread_mutex_lock(&bufferMutex);
		}

		// Go through all buffer slots and find an empty one;
		int i;
		for (i = 0; i < MAXSLOTS; i++) {
			if (buffer[i].isFilled == FALSE) {
				buffer[i].isFilled = TRUE;
				buffer[i].workerthread_id = in_workerthread_id;
				buffer[i].clientRequest = clientRequest;
				buffer[i].priority = clientRequest->priority;
				buffer[i].messageID = messageID;
				buffer[i].pixArray = in_pixArray;
				buffer[i].rows = in_rows;
				buffer[i].cols = in_cols;
				buffer[i].frame = in_frame;
				buffer[i].repeat = in_repeat;
				openSlots--;

				// Check to see if m slots are full, then signal dispatcher
				if (openSlots == (MAXSLOTS - DISPATCHER_BATCH_AMOUNT)) {
					pthread_cond_signal(&dispatcherWait);

					// Have the worker thread wait until the dispatcher wakes it
					pthread_cond_wait(&bufferWait, &bufferMutex);
				}

				// Release buffer mutex.
				pthread_mutex_unlock(&bufferMutex);
				return;
			}
		}
		// Release buffer mutex.
		pthread_mutex_unlock(&bufferMutex);
	}
}


// Generates the messages based on the function requested by the client
// Function name is either: start_movie, stop_movie, seek_movie
void generateMessages(int workerThreadID, request* req) {
	int frame, messageID, endFrame, startFrame;
	int repeat;
	char* clientID = req->clientID;

	// Handle a stop_movie request
	// Add the clientID into the stopList
	if(strcmp(req->funcRequest, "stop_movie") == 0) {\
		insertStopID(clientID);
#ifdef DEBUG
		printStopList();
#endif
		return;
	}

	// Handle a start_movie request
	if (strcmp(req->funcRequest, "start_movie") == 0) {
		startFrame = 1;  
		repeat = req->argument; 
	}
	// Handle a seek_movie request
	else if(strcmp(req->funcRequest, "seek_movie") == 0) {
		// Start frame is the seek frame
		startFrame = req->argument;
		repeat = 0;
		// Do some minor error checking
		if (startFrame <= 0) {
			startFrame = 1;
		}
	}

	messageID = 1;
	endFrame = 100; //magic cookie bad

	// Create a message for each frame from the startframe to the endframe
	// For start_movie, it is the entire movie
	// For seek_movie, it is seekFrame-->endFrame
	for (frame = startFrame; frame <= endFrame; frame++) {
		pixel** pixArray;
		FILE *fp;
		int cols, rows;
		pixval maxval;
		char s[80];

		// Attempt to open the frame for processing
		sprintf (s, "./images/%s%d.ppm", req->filePrefix, frame);
		if ((fp = fopen (s,"r")) == NULL) {
			fprintf (stderr, "%s: Can't open input file:\n %s.\n", s);
			exit (1);
		}

		// Read the ppm image into a multidimensional array
		pixArray = ppm_readppm (fp, &cols, &rows, &maxval);
		fclose (fp);

		// Since all slots have a copy of the original client request
		// create a pointer to the request object so we don't waste memory
		request* copyReq = req;

		// IMPORTANT regarding stopping playback	
		// We check every 20 frames to reduce redundancy
		// We don't want to generate messages for a movie that the client
		// has already requested be stopped, so check every 20 messages to make sure
		if (frame != 100 && ((frame % 20) == 0)) {
			// If the message is part of a movie which the client has already
			// requested be stopped, then stop sending them frames and
			// jump to the last frame
			if (searchStopList(clientID) == TRUE) {
				frame = 99;	
				ppm_freearray (pixArray, rows);
			}
			// Otherwise this is still part of a valid movie request
			else {
				// Insert this message into the buffer	and increment the message id
				insertToBuffer(workerThreadID, copyReq, messageID, pixArray, rows, cols, frame, repeat);
				messageID++;
			}
		}
		else {
			// Insert this message into the buffer	and increment the message id
			insertToBuffer(workerThreadID, copyReq, messageID, pixArray, rows, cols, frame, repeat);
			messageID++;
		}
	}                                                                           
}


// Helper function for qsort().
int compare(struct slot* a, struct slot* b) {
	return (a->priority < b->priority);
}


// Initialize one slot in the circular buffer
void initSlot(int slotNumber) {
	buffer[slotNumber].isFilled = FALSE;
	buffer[slotNumber].workerthread_id = -1;
	buffer[slotNumber].priority = -1;
	buffer[slotNumber].messageID = -1;
	buffer[slotNumber].clientRequest = NULL;
	buffer[slotNumber].pixArray = NULL;
	buffer[slotNumber].rows = -1;
	buffer[slotNumber].cols = -1;
	buffer[slotNumber].repeat = -1;
	buffer[slotNumber].frame = -1;
}


// Initialize the circular buffer
void initBuffer() {
	int i;
	for (i = 0; i < MAXSLOTS; i++) {
		initSlot(i);
	}
}


// Sends the pixarray for a given slot in the circular buffer
// The size of the frame we send is (rows * cols * 3) + 1.
// The *3 is for the three channels per pixel (RGB) and the +1
// is an extra byte which lets the client know whether or not
// more data is coming (we denote this byte as the TERM flag).
// If the term flag == 1, then there is more data coming, so the
// client should keep the connection open.
// If the term flag == 0, then there is no more data coming, and
// the client should close the connection.
void sendFrame(int bufferIndex) { 
	int i = bufferIndex;
	int sizeToSend = buffer[i].rows * buffer[i].cols * 3 + 1;
	int x, y;
	int rows = buffer[i].rows;
	int cols = buffer[i].cols;
	unsigned char *buf = NULL;

	// +1 slot used to determine if there is more data coming: 0 or 1
	buf = (unsigned char *)malloc ((cols*rows*3) + 1);
	if (!buf) {
		perror("Could not malloc memory for buf\n");	
		exit(1);
	}

	// Place the frame data into a char array
	for (y = 0; y < rows; y++) {
		for (x = 0; x < cols; x++) {
			buf[(y*cols+x)*3+0] = PPM_GETR(buffer[i].pixArray[rows-y-1][x]);
			buf[(y*cols+x)*3+1] = PPM_GETG(buffer[i].pixArray[rows-y-1][x]);
			buf[(y*cols+x)*3+2] = PPM_GETB(buffer[i].pixArray[rows-y-1][x]);
		}
	}
	char* clientID = buffer[i].clientRequest->clientID;

	// Determine if this is part of a movie which was already stopped by client
	int inStopList = searchStopList(clientID);

	// clientRequest objects are shared for all frames belonging to a single
	// client request.  Therefore, no need to make redundant copies, just pointers.
	// Only free this client request obj when the last frame is serviced!
	int deleteFlag = FALSE;

	// Check to see if the movie has repeat = 1, if so, there is more data to send even
	// though the movie is done playing.
	if (buffer[i].frame == 100) {
		// If its a repeat, and the client hasnt requested a stop movie, then copy this request again
		if (buffer[i].repeat == TRUE && inStopList == FALSE) {
			// Add a start_movie request to the request list, with repeat flag = 1
			// Set the 100th frame with more data flag = 1
			deleteFlag = FALSE;
			insertRequest(buffer[i].clientRequest);
			buf[cols*rows*3] = NOTCLOSE;
		}
		else {
			// Call deleteStopID(clientID)
			// Set the 100th frame with more data flag = 0
			deleteStopID(clientID);
			buf[cols*rows*3] = CLOSE;
			deleteFlag = TRUE;
		}
	}
	// If its not the last frame
	else {
		// If it is part of a movie which was stopped, then this is the last frame
		// and the client should just close the connection
		if (inStopList == TRUE) {
			buf[cols*rows*3] = CLOSE;
		}
		// Otherwise this is a frame for a movie which has regular playback
		// so don't close the connection, just receive the movie frames
		else {
			buf[cols*rows*3] = NOTCLOSE;
		}
	}

	// The movie hasn't been stopped, so send the frame data
	if (inStopList == FALSE) {
		// Send the entire frame (reduces write syscalls)
#ifdef DEBUG
		printf("Send frame: %d    request string: %s\n", 
				buffer[i].frame, buffer[i].clientRequest->requestString);
#endif
		write(buffer[i].clientRequest->clientSD, buf, sizeToSend);
		if (errno == EPIPE) {
#ifdef DEBUG
			printf("Got EPIPE error   errno: %d\n", errno);
#endif
			insertStopID(buffer[i].clientRequest->clientID);
			close(buffer[i].clientRequest->clientSD);
			errno = 0;
		}
	}

	// Release the memory 
	ppm_freearray (buffer[i].pixArray, buffer[i].rows);
	free(buf);

	// If this clientRequest obj is not going to be used anymore, free
	if (deleteFlag == TRUE) {
		free(buffer[i].clientRequest);
	}
}


// Dispatcher thread's function call that processes m messages.
void process() {
	// Do this forever
	while (1) {
#ifdef DEBUG
		printf("dispatcher: going to sleep again\n");
#endif

		// Wait until its time to wake up
		pthread_cond_wait(&dispatcherWait, &bufferMutex);

#ifdef DEBUG
		printf("dispatcher: i'm awake\n");
#endif

		// Ok at this point our dispatcher is awake and ready

		// Dispatcher thread calls sorting algorithm here to sort buffer in order.
		qsort(buffer, MAXSLOTS, sizeof(struct slot), compare);
#ifdef DEBUG
		printf("dispatcher: buffer sorted\n");
#endif

		// Retrieve m messages
		int i = 0;
		int served = 0;
		while (served < DISPATCHER_BATCH_AMOUNT && i < MAXSLOTS) {
			if (buffer[i].isFilled == TRUE) {
				// PART 1 Test
				//	write(buffer[i].clientRequest->clientSD, buffer[i].clientRequest->requestString, 1000);

				// Send this frame
				sendFrame(i);

				openSlots++;
				initSlot(i);
				served++;
			}
			i++;
		}
#ifdef DEBUG
		printf("dispatcher: done\n");
#endif

		// Release buffer mutex.
		pthread_mutex_unlock(&bufferMutex);

		// Let workers know I'm done so they can continue
		// Prevents a deadlock I was getting
		pthread_cond_signal(&bufferWait);
	}
}



