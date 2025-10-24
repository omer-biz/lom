run: build
	./build/main

build: main.c
	mkdir ./build/
	gcc -I./include/ -L./lib/ -o ./build/main main.c

clean:
	rm -rf ./build
