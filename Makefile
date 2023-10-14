CC = gcc
CFLAGS = -Wall

all: master slave

master: master.c
	$(CC) $(CFLAGS) master.c -o master

slave: slave.c
	$(CC) $(CFLAGS) slave.c -o slave

clean:
	rm -f master slave logfile.* cstest
