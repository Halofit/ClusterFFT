
#Compiler to use
CC=mpiCC

#Flags to use
CFLAGS=-O3 -Wall -std=c++1y

all: clusterFFT

clusterFFT: Main.o ComplexArr.o 
	$(CC) $(CFLAGS) Main.o ComplexArr.o -o clusterFFT

Main.o: Main.cpp
	$(CC) $(CFLAGS) -c Main.cpp

ComplexArr.o: ComplexArr.cpp
	$(CC) $(CFLAGS) -c ComplexArr.cpp

clean:
	rm *.o clusterFFT
