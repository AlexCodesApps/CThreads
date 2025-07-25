.PHONY: donotcall

build/threads.o: build build/script threads.c threads.h
	./build/script

donotcall:
	$(CC) -c threads.c -o build/threads.o -std=c11 $(PTHREAD)

build:
	mkdir build

build/script: build script.c
	$(CC) script.c -o build/script
