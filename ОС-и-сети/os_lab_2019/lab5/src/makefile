CC=gcc
CFLAGS=-Wall

all: mutex_no_lock mutex_with_lock parallel_factorial deadlock

# Задание 1: одна и та же программа без мьютекса и с мьютексом
mutex_no_lock : mutex.c
	$(CC) $(CFLAGS) -o mutex_no_lock mutex.c -DUSE_MUTEX=0 -lpthread

mutex_with_lock : mutex.c
	$(CC) $(CFLAGS) -o mutex_with_lock mutex.c -DUSE_MUTEX=1 -lpthread

parallel_factorial : parallel_factorial.c
	$(CC) $(CFLAGS) -o parallel_factorial parallel_factorial.c -lpthread

deadlock : deadlock.c
	$(CC) $(CFLAGS) -o deadlock deadlock.c -lpthread

clean :
	rm -f mutex_no_lock mutex_with_lock parallel_factorial deadlock
