all:
	gcc src/threadpool.c  -c -Wall -lnetpbm
	gcc src/buffer.c -c -Wall -lnetpbm 
	gcc server.c threadpool.o buffer.o -o server -Wall -lpthread -lnetpbm
	gcc -I./include -L/usr/X11R6/lib -L./lib/GL -L./lib/netpbm client.c -lGLU -lGL -lnetpbm -lXext -lX11 -lm -lpthread -lnetpbm -o client
	rm threadpool.o buffer.o

clean:
	rm server 
	rm client 
	rm src/threadpool.o 
	rm src/buffer.o

