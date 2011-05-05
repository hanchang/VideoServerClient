#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "src/datatypes.h"
#include "src/threadpool.h"
#include "src/buffer.h"

extern int requestID;


// Checks if the requested movie exists on the server
// Poorly designed: should check against a Database or File
// but for the assignment this seems to work
int isValidMovie(request* clientRequest) {
	if (!clientRequest) {
		return FALSE;
	}
	
	if (strcmp(clientRequest->movieName, "starwars") == 0) {
		clientRequest->filePrefix = "sw";
		return TRUE;
	}

	// Can add else ifs to check for other new movies as they are added
	return FALSE;
}


// Server loop
void servConn (int port) {
	int sd, new_sd;
  	struct sockaddr_in name, cli_name;
  	int sock_opt_val = 1;
  	int cli_len;
  	char data[1000];		/* Our receive data buffer. */
  
  	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
   	perror("(servConn): socket() error");
    	exit (-1);
  	}

  	if (setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_opt_val,
		sizeof(sock_opt_val)) < 0) {
    	perror ("(servConn): Failed to set SO_REUSEADDR on INET socket");
    	exit (-1);
  	}

  	name.sin_family = AF_INET;
  	name.sin_port = htons (port);
  	name.sin_addr.s_addr = htonl(INADDR_ANY);
  
  	if (bind (sd, (struct sockaddr *)&name, sizeof(name)) < 0) {
    	perror ("(servConn): bind() error");
    	exit (-1);
  	}

  	listen (sd, 5);

 	for (;;) {
   	cli_len = sizeof (cli_name);
      new_sd = accept (sd, (struct sockaddr *) &cli_name, &cli_len);
      printf ("Assigning new socket descriptor:  %d\n", new_sd);
      
      if (new_sd < 0) {
			perror ("(servConn): accept() error");
			exit (-1);
      }
			
		/* At this point we have a new client connection.  The server should:
		 *		1) Get the client request
		 *		2) Create a new request object with the client request
		 *		3) Signal that a new request is pending, which wakes a worker thread
		 *		4) Worker thread gets the next request from the request list
		 *		5) Handles request by trying to place it into circular buffer
		 */
		
		// Get the client request string
		read(new_sd, &data, 1000);
		printf("Received request: %s\n", data);
		
		// Create a new request object with the client request
		request* newRequest = (request*) malloc(sizeof(request));
		if (!newRequest) {
			perror("Could not malloc memory for new request.\n");
			exit(1);
		}
		
		// Initialize the request obj
		initRequest(newRequest);

		// Populate the request obj with the client request string
		populateRequest(data, newRequest);
		newRequest->number = requestID;
	
		// Increment the super-increasing request ID
		requestID++;
	
		// Set the client socket descriptor
		newRequest->clientSD = new_sd;
	
		// Check if the requested movie exists on the server
		int errVal;
		if (!isValidMovie(newRequest)) {
			// Tell the client movie does not exist and exit
			errVal = 0;
			int err = write(new_sd, (char*)&errVal, sizeof(int));
			if (err == EPIPE) {
				close(new_sd);
			}
			free(newRequest);
		}
		else {
			// Tell the client the movie does exist 	
			errVal = 1;
			int err = write(new_sd, (char*)&errVal, sizeof(int));
			if (err == EPIPE) {
				close(new_sd);
			}
			// Insert the new request into the request linked list
			insertRequest(newRequest);	
		}
	}
}


int main(int argc, char** argv) {
	if (argc != 2) {
		perror("usage: ./server [num_worker_threads]\n");
		return 0;
	}

	signal(SIGPIPE, SIG_IGN);

	int numWorkers = atoi(argv[1]);

	// Create initial worker threads
	printf("Initializing thread pool.\n");	
	initThreadPool(numWorkers);

	printf("Initializing circular buffer.\n");
	initBuffer();

	printf ("Server enabled; waiting for connections... \n");
  	servConn (7038);		/* Server port. */

  return 0;
}



