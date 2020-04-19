ifeq ($(CC),)
   	CC = $(CROSS_COMPILE)gcc
endif

ifeq ($(CFLAGS),)
   	CFLAGS = -g -Wall -Werror
endif



# first target all- default target. Calls native gcc compiler if Cross_COMPILE not described

all:
	$(CC) $(CFLAGS) client.c -o client
	$(CC) $(CFLAGS) server.c -o server 
clean:
	rm -f client
	rm -f server
