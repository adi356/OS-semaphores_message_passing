CC = gcc
CFLAGS = -Wall

all: master slave

.SUFFIXES: .c .o

.c:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f master slave logfile.* cstest

