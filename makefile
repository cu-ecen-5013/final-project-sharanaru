ifeq ($(CC),)
   	CC = $(CROSS_COMPILE)gcc
endif

ifeq ($(CFLAGS),)
   	CFLAGS = -g -Wall -Werror
endif

ifeq ($(LDFLAGS),)
   	LDFLAGS = -lrt
endif

# first target all- default target. Calls native gcc compiler if Cross_COMPILE not described

all:
	$(CC) $(CFLAGS) client.c -o client $(LDFLAGS)
	$(CC) $(CFLAGS) server.c -o server $(LDFLAGS) 
clean:
	rm -f client
	rm -f server
