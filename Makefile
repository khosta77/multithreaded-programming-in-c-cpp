CC=g++

all: clean main ./threadpool.out

main:
	@$(CC) -pthread -std=c++17 threadpool.cpp -o threadpool.out

clean:
	@rm -rf *.out
