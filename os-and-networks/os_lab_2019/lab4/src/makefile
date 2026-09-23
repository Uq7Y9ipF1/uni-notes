CC=gcc
CFLAGS=-I. -Wall

# Задание 4 (и бонусом сборка psum из задания 6)
all: parallel_min_max process_memory zombie psum

parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h parallel_min_max.c
	$(CC) $(CFLAGS) -o parallel_min_max parallel_min_max.c utils.o find_min_max.o

process_memory : process_memory.c
	$(CC) -o process_memory process_memory.c

zombie : zombie.c
	$(CC) -o zombie zombie.c

utils.o : utils.h
	$(CC) -c -o utils.o utils.c $(CFLAGS)

find_min_max.o : utils.h find_min_max.h
	$(CC) -c -o find_min_max.o find_min_max.c $(CFLAGS)

# Задание 6: параллельная сумма линкуется со статической библиотекой libsum.a
psum : parallel_sum.c libsum.a
	$(CC) $(CFLAGS) -o psum parallel_sum.c libsum.a utils.o -lpthread

libsum.a : sum.o
	ar rcs libsum.a sum.o

sum.o : sum.h sum.c
	$(CC) -c -o sum.o sum.c $(CFLAGS)

clean :
	rm -f parallel_min_max process_memory zombie psum utils.o find_min_max.o sum.o libsum.a tmp_*.txt
