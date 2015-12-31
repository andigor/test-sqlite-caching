main : main.o
	g++ -g $^ -o $@ -lsqlite3 -std=c++11 -pthread

main.o : main.cpp
	g++ -g $< -c -o $@ -std=c++11 -pthread


