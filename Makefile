all: clean main threadpool

main:
	g++ -pthread -std=c++17 threadpool.cpp -o threadpool.out

clean:
	rm -rf *.out

threadpool:
	./threadpool.out
