CC = g++
CFLAGS1 = -std=c++11 -c
CFLAGS2 = -std=c++11 -O3
all:
	$(CC) $(CFLAGS1) 2048.cpp
	$(CC) $(CFLAGS2) main.cpp 2048.o -o agent_2048
clean:
	rm 2048.o agent_2048
