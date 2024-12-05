all: clean main threadpool

main:
	g++ -pthread -std=c++17 main.cpp -o main.out

clean:
	rm -rf *.out

# Мы так от TARGET ушли по итогу)))))))))))))))))
main.out:
	./main.out
