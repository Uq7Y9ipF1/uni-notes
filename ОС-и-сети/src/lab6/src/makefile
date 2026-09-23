CC=gcc
CFLAGS=-I. -Wall -Wextra
LDFLAGS=-lpthread

# Задание 2: makefile для клиента и сервера.
# Задание 3: дублирующийся код (MultModulo, ConvertStringToUI64) вынесен в
# статическую библиотеку libcommon.a, с которой линкуются обе программы.
all: client server

libcommon.a : common.o
	ar rcs libcommon.a common.o

common.o : common.h
	$(CC) -c common.c -o common.o $(CFLAGS)

client : client.c common.o libcommon.a
	$(CC) -o client client.c libcommon.a $(CFLAGS) $(LDFLAGS)

server : server.c common.o libcommon.a
	$(CC) -o server server.c libcommon.a $(CFLAGS) $(LDFLAGS)

clean :
	rm -f client server common.o libcommon.a

.PHONY : all clean
