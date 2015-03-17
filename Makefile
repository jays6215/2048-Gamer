all:
	g++ -std=c++11 -c 2048.cpp
	g++ -std=c++11 -O3 main.cpp 2048.o -o agent_2048
