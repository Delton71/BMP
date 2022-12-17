CC=clang++
CFLAGS=-lboost_system -lboost_filesystem
RES=main.cpp console.cpp

all:
	@$(CC) $(RES) $(CFLAGS) -o BMP
	./BMP
