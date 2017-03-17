all: build

build:
	g++ src/lsm.cpp src/bloom.cpp -o bin/lsm -std=c++11 -g

generator:
	$(CC) generator/generator.c -o bin/generator -I/usr/local/include -L/usr/local/lib -lgsl -lgslcblas

clean:
	rm bin/lsm bin/generator
