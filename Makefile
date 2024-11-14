CC=g++
STD=-std=c++20
W=-Wall -Werror -Wextra
SANITIZE=-fsanitize=address

all: clean server client

server:
	$(CC) $(STD) -O2 $(W) server.cpp -o server.out

client:
	$(CC) $(STD) -O2 $(W) client.cpp -o client.out

clean:
	rm -rf *.out
