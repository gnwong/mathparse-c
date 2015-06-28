# (c) 2015 George Wong

CC = gcc
FLAGS = -Wall -g

all: mpc.o
	gcc main.c mpc.o -o main

test:
	./main

%.o: %.c
	$(CC) $(FLAGS) -c $<

clean:
	- rm *.o
