
#Compiler to use
CC=mpiCC

#Flags to use
CFLAGS=-O3 -Wall -std=c++1y -lsndfile

all: clusterFFT.out

clusterFFT.out: Main.o ComplexArr.o 
	$(CC) $(CFLAGS) Main.o ComplexArr.o -o clusterFFT.out

Main.o: src/MPI/Main.cpp
	$(CC) $(CFLAGS) -c src/MPI/Main.cpp

ComplexArr.o: src/MPI/ComplexArr.cpp
	$(CC) $(CFLAGS) -c src/MPI/ComplexArr.cpp

