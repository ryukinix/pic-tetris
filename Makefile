build: main.c
	gcc -o main main.c -lSDL

run: build
	./main

clean:
	rm -rf main

.PHONY: clean
