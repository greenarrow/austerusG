GCC = gcc -Wall

all: objdir austerus-core

objdir:
	mkdir -p build

austerus-core: serial.o austerus-core.o
	$(GCC) -o austerus-core build/serial.o build/austerus-core.o

austerus-core.o: src/austerus-core.c
	$(GCC) -c -o build/austerus-core.o src/austerus-core.c

serial.o: src/serial.c
	$(GCC) -c -o build/serial.o src/serial.c

clean:
	rm austerus-core build/*

