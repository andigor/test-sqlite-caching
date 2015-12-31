.PHONY: all clean

all: main_64 main_32

main_64 : main_64.o
	g++ -g $^ -o $@ -lsqlite3 -std=c++11 -pthread -m64
main_64.o : main.cpp
	g++ -g $< -c -o $@ -std=c++11 -pthread -m64


main_32 : main_32.o
	g++ -g $^ -o $@ -lsqlite3 -std=c++11 -pthread -m32
main_32.o : main.cpp
	g++ -g $< -c -o $@ -std=c++11 -pthread -m32

clean:
	rm -rf main_32 main_64 main_32.o main_64.o
