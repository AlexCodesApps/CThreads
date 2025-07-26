.PHONY: clean

build/libCThreads.a: build/Makefile threads.h threads.c
	make -C build

build/Makefile: CMakeLists.txt
	cmake -S . -B build -G "Unix Makefiles"

clean:
	rm -rf build
