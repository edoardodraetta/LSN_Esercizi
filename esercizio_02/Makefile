CC = g++
CFLAGS = -Wall -O3 --std=c++11

main.exe : main.o random.o RandomWalk.o statistics.o
	$(CC) random.o statistics.o RandomWalk.o main.o -o main.exe
main.o : main.cpp
	$(CC) -c main.cpp -o main.o $(CFLAGS)
statistics.o : statistics.cpp statistics.h
	$(CC) -c statistics.cpp -o statistics.o $(CFLAGS)
RandomWalk.o : RandomWalk.cpp RandomWalk.h random.h
	$(CC) -c RandomWalk.cpp -o RandomWalk.o $(CFLAGS)
random.o : random.cpp random.h
	$(CC) -c random.cpp -o random.o $(CFLAGS)
clean :
	rm *.o main.exe seed.out
