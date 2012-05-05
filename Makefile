CC = gcc -Wall

all: objdir austerus-send austerus-core

objdir:
	mkdir -p build

austerus-send: stats.o serial.o austerus-send.o
	$(CC) -o austerus-send build/stats.o build/serial.o \
		build/austerus-send.o

austerus-core: serial.o austerus-core.o
	$(CC) -o austerus-core build/serial.o build/austerus-core.o

austerus-send.o: src/austerus-send.c
	$(CC) -c -o build/austerus-send.o src/austerus-send.c

austerus-core.o: src/austerus-core.c
	$(CC) -c -o build/austerus-core.o src/austerus-core.c

serial.o: src/serial.c
	$(CC) -c -o build/serial.o src/serial.c

stats.o: src/stats.c
	$(CC) -c -o build/stats.o src/stats.c

popen2.o: src/popen2.c
	$(CC) -c -o build/popen2.o src/popen2.c

clean:
	rm austerus-send austerus-core build/*

