CC=gcc

all: clean main

main:
	$(CC) -pthread main.c -o main.out

clean:
	rm -rf *.out
