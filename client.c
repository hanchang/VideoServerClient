#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <stdlib.h>
#include <GL/glx.h>
#include <X11/keysym.h>
#include <ppm.h>

Display *dpy;
Window window;

// -- West's tcp_client file. ---------------------------------------------
// ------------------------------------------------------------------------
int cliConn (char *host, int port) {
 
  struct sockaddr_in name;
  struct hostent *hent;
  int sd;
 
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("(cliConn): socket() error");
    exit (-1);
  }
 
  if ((hent = gethostbyname (host)) == NULL)
    perror("Host not found.\n");
  else
    bcopy (hent->h_addr, &name.sin_addr, hent->h_length);
 
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
 
  /* connect port */
  if (connect (sd, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror("(cliConn): connect() error");
    exit (-1);
  }
 
  return (sd);
}

// -- ImageView / XWin code ----------------------------------------
// -----------------------------------------------------------------

static int attributeList[] = { GLX_RGBA, GLX_RED_SIZE, 1, None };

void noborder (Display *dpy, Window win) {
  struct {
    long flags;
    long functions;
    long decorations;
    long input_mode;
  } *hints;
  
  int fmt;
  unsigned long nitems, byaf;
  Atom type;
  Atom mwmhints = XInternAtom (dpy, "_MOTIF_WM_HINTS", False);

  XGetWindowProperty (dpy, win, mwmhints, 0, 4, False, mwmhints,
		      &type, &fmt, &nitems, &byaf, 
		      (unsigned char**)&hints);
  
  if (!hints) 
    hints = (void *)malloc (sizeof *hints);
  
  hints->decorations = 0;
  hints->flags |= 2;
  
  XChangeProperty (dpy, win, mwmhints, mwmhints, 32, PropModeReplace,
		   (unsigned char *)hints, 4);
  XFlush (dpy);
  free (hints);
}


static void make_window (int width, int height, char *name, int border) {
  XVisualInfo *vi;
  Colormap cmap;
  XSetWindowAttributes swa;
  GLXContext cx;
  XSizeHints sizehints;
  
  dpy = XOpenDisplay (0);
  if (!(vi = glXChooseVisual (dpy, DefaultScreen(dpy), attributeList))) {
    printf ("Can't find requested visual.\n");
    exit (1);
  }
  cx = glXCreateContext (dpy, vi, 0, GL_TRUE);

  swa.colormap = XCreateColormap (dpy, RootWindow (dpy, vi->screen),
				  vi->visual, AllocNone);
  sizehints.flags = 0;
  
  swa.border_pixel = 0;
  swa.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask;
  window = XCreateWindow (dpy, RootWindow (dpy, vi->screen), 
			  0, 0, width, height,
			  0, vi->depth, InputOutput, vi->visual,
			  CWBorderPixel|CWColormap|CWEventMask, &swa);
  XMapWindow (dpy, window);
  XSetStandardProperties (dpy, window, name, name,
			  None, (void *)0, 0, &sizehints);
  
  if (!border) 
    noborder (dpy, window);
  
  glXMakeCurrent (dpy, window, cx);
}

// -- Client User Interface -----------------------------------------------
// -----------------------------------------------------------------------

static int thread_count = 0;
static int client_count = 0;
static int exit_flag = 0;

char* generateClientID(int);
void init();
void stopThreadInit();
void stopMenu();
void movieplayer(int, int, char*);
int mainMenu();
int argMenu(char);
char* movieMenu();
int pickClientID(int);

#define START 's'
#define STOP 'p'
#define SEEK 'k'
#define EXIT 'e'

#define DELIMITER ":"

#define SERVER "csa3.bu.edu"
#define PORT 7038

void init() {
    char req;
    int arg;
    req = mainMenu();
	
	if (req == EXIT)
		return;

	if (req != STOP) {	
        arg = argMenu(req);
   	    char* movie = (char*)movieMenu();
       	movieplayer(req, arg, movie);
	}
	else
		stopMenu();
}


int mainMenu() {
	char req;

	printf("Welcome! Please choose from the following menu: \n");
    printf("- (S)tart a movie.\n");
    printf("- Sto(p) a movie. \n");
    printf("- See(k) to a frame. \n");
	printf("- (E)xit.\n");
    printf("Enter request: ");

    while (req != START && req != STOP && req != SEEK && req != EXIT) 
		scanf("%c", &req);

	if(req == EXIT)
		exit_flag = 1;

    return req;
}


int argMenu(char req) {
        int ret = 0;
		char repeat;

        if (req == START) {
                printf("Repeat movie? (Y)es or (N)o? ");
                while (repeat != 'y' && repeat != 'n') {
                    scanf("%c", &repeat);
                }
        }

        if (req == SEEK) {
                printf("Please enter the frame number to seek: ");
                scanf("%d", &ret);
        }

		if (repeat == 'y')
			ret = 1;
		if (repeat == 'n')
			ret = 0;

        //printf("Argument chosen: %d\n", ret);
        return ret;
}


char* movieMenu() {
        char movie[100];
        printf("Please enter the name of the movie you would like: \n");
        scanf("%s", movie);
        printf("Now playing: %s.\n", movie);
        return movie;
}

void stopMenu() {
	int var = 0;
	char req;
    char* client_id;
    char priority[] = "1";
    char* request;
    char* movie = "starwars";
    int arg = 0;
    char string_arg[25];
    sprintf(string_arg, "%d", arg);

	printf("Please choose from the following menu: \n");
    printf("- (B)ack to main menu\n");
    printf("- (S)top a movie\n");
	printf("Enter request: ");
    while(req != 'b' && req != 's')
     	scanf("%c", &req);

	if (req == 'b')
		var = 0;
	if (req == 's')
		var = 1;

    char message[1000] = "";

    if(var == 0) {
        int ret;
        pthread_t newMovie;
        ret = pthread_create(&newMovie, NULL, (void *) init, NULL);
    }

    if(var == 1) {
        request = "stop_movie";

        client_id = generateClientID(chooseStopMenu());

        strcat(message, client_id);
        strcat(message, DELIMITER);
        strcat(message, priority);
        strcat(message, DELIMITER);
        strcat(message, request);
        strcat(message, DELIMITER);
        strcat(message, movie);
        strcat(message, DELIMITER);
        strcat(message, string_arg);

        int sd = cliConn (SERVER, PORT);
        write (sd, message, 1000);

        thread_count--;

        int ret;
        pthread_t newMovie;
        ret = pthread_create(&newMovie, NULL, (void *) init, NULL);
    }
}

void movieplayer(int req, int arg, char* movie) {
    thread_count++;
	client_count++;
    char* client_id = generateClientID(client_count);
    char priority[] = "1";
    char* request;
    char string_arg[100];

	// ImageView variables.
   	register pixel** pixarray;
   	int cols, rows;
   	register int retx, y;
	unsigned char *buf=NULL;
   	int border = 1;
   	int frame_count = 1;
   	char s[80];

    // Parsing request number into a request string.
    if (req == START)
        request = "start_movie";

    if (req == STOP)
        request = "stop_movie";

    if (req == SEEK)
        request = "seek_movie";


    // Parsing (int) argument into a string as well.
    sprintf(string_arg, "%d", arg);


    // Parsing an entire message of the required form.
    char message[1000] = "";
    strcat(message, client_id);
    strcat(message, DELIMITER);
	strcat(message, priority);
    strcat(message, DELIMITER);
    strcat(message, request);
    strcat(message, DELIMITER);
    strcat(message, movie);
    strcat(message, DELIMITER);
    strcat(message, string_arg);

    int sd = cliConn (SERVER, PORT);

    int totalSize = 160 * 120 * 3 + 1;
    char* buffer = (char *) malloc(sizeof (char) * totalSize);
    write (sd, message, 1000);

	int errorchecking = -1;
	read(sd, &errorchecking, (sizeof (int)));
	if (errorchecking != 1) {
		printf("Unfortunately, we don't have the movie you requested.\n");
		printf("Please choose another movie at the main menu.\n");
		pthread_t menuThread;
		pthread_create(&menuThread, NULL, init, NULL);
		pthread_exit(1);
	}

    make_window (160, 120, generateClientID(client_count), 1);
    glMatrixMode (GL_PROJECTION);
    glOrtho (0, cols, 0, rows, -1, 1);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glMatrixMode (GL_MODELVIEW);
    glRasterPos2i (0, 0);

  	int ret;
   	pthread_t stopThread;
    ret = pthread_create(&stopThread, NULL, (void *) stopMenu, NULL);

    int frameCount = 0;
    int bytesRead = 0;
    char* newbuffer = (char*) malloc(sizeof (char) * (totalSize-1));
    do {
    	bytesRead = recv(sd, buffer, totalSize, MSG_WAITALL);
		memcpy(newbuffer, buffer, totalSize);
	        
		// ImageView maker.
        glDrawPixels (160, 120, GL_RGB, GL_UNSIGNED_BYTE, newbuffer);
        glFlush ();
        frameCount++;
    }	
	while (buffer[totalSize-1] != 0) ;
                
	close(sd);
    free(buffer);
    free(newbuffer);
	pthread_exit(0);
}


int chooseStopMenu() {
	int id;
	printf("Please choose the movie to stop:\n");
	printf("(Use the number in the MoviePlayer window.)\n");
	scanf("%d", &id);

	return(id);
}


char* generateClientID(int client_id) {
    char output[100];
    char hostname[100];

    // Change client_id into string.
    sprintf(output, "%d", client_id);
    gethostname(hostname, 100);
    strcat(output, hostname);
    return (output);
}

int main() {
    printf("\nStarting MoviePlayer client...\n");
	printf("Connecting to server %s on port %d...\n", SERVER, PORT);

    pthread_t client;
    pthread_attr_t tattr;
    int ret;

    printf("\nMoviePlayer initialized!\n\n");

    ret = pthread_attr_setschedpolicy(&tattr, SCHED_RR);
    ret = pthread_create(&client, NULL, (void *) init, NULL);
    
    while(exit_flag == 0) { }
    
    return 0;
}

