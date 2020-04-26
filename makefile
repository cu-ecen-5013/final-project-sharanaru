ifeq ($(CC),)
   	CC = $(CROSS_COMPILE)gcc
endif

ifeq ($(CFLAGS),)
   	CFLAGS = -g -Wall -Werror
endif

ifeq ($(LDFLAGS),)
   	LDFLAGS = -lrt -pthread -lm
endif

# first target all- default target. Calls native gcc compiler if Cross_COMPILE not described

all:
	$(CC) $(CFLAGS) client.c -o client $(LDFLAGS)
	$(CC) $(CFLAGS) server.c -o server $(LDFLAGS)
	$(CC) $(CFLAGS) server-test.c -o servertest
	$(CC) $(CFLAGS) i2ctest.c -o sensor $(LDFLAGS)  
clean:
	rm -f client
	rm -f server
	rm -f servertest
	rm -f sensor
