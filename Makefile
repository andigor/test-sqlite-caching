main : main.o
	g++ -g $^ -o $@ -lsqlite3

main.o : main.cpp
	g++ -g $< -c -o $@


