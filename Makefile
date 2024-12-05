all: clean main threadpool

main:
	g++ -pthread -std=c++20 threadpool.cpp -o threadpool.out

clean:
	rm -rf *.out

threadpool:
	./threadpool.out
