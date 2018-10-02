array: main.c
	gcc -std=gnu99 -Os -DARRAY -o test.run main.c

ll: main.c
	gcc -std=gnu99 -Os -o test.run main.c

clean:
	rm -f ./test.run
