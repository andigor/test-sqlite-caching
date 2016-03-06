.PHONY: all clean

#all: main_64 main_32
all: main_64 

SQLITE_DIR := sqlite-amalgamation-3100200

main_64 : main_64.o sqlite_64.o
	g++ -g -O3 $^ -o $@ -std=c++11 -pthread -m64 -ldl

main_64.o : main.cpp $(SQLITE_DIR)/sqlite3.h
	g++ -g -O3 $< -c -o $@ -std=c++11 -pthread -m64 -I$(SQLITE_DIR)

sqlite_64.o : $(SQLITE_DIR)/sqlite3.c $(SQLITE_DIR)/sqlite3.h
	gcc -g $< -c -o $@ -pthread -m64 -I$(SQLITE_DIR) -DSQLITE_THREADSAFE=2


#main_32 : main_32.o
#	g++ -g $^ -o $@ -std=c++11 -pthread -m32
#main_32.o : main.cpp
#	g++ -g $< -c -o $@ -std=c++11 -pthread -m32 -Isqlite-amalgamation-3100200

clean:
	rm -rf main_32 main_64 main_32.o main_64.o
