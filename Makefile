CFLAGS = -g -Wall -O2 -I./libssh2/include
CC = gcc

all: sftp

sftp: sftp.o
	$(CC) -o $@ $^ -L./libssh2/src/.libs -lssh2

clean:
	$(RM) *.o
	$(RM) *~
	$(RM) *#
