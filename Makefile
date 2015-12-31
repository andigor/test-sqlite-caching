main : main.o
	g++ -g $^ -o $@ -lsqlite3 -std=c++11

main.o : main.cpp
	g++ -g $< -c -o $@ -std=c++11


