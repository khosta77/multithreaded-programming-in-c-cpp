all: clean main pinpong

main:
	g++ -pthread -std=c++20 pingpong.cpp -o pinpong.out

clean:
	rm -rf *.out

pinpong:
	./pinpong.out
