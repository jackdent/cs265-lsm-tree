all: build

build:
	g++ src/*.cpp -o bin/lsm -std=c++11 -I./lib -I/usr/local/include -L/usr/local/lib -l boost_system -g

generator:
	gcc generator/generator.c -o bin/generator -I/usr/local/include -L/usr/local/lib -lgsl -lgslcblas

clean:
	rm bin/lsm bin/generator
