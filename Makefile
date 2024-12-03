compile: mysh.c
	gcc mysh.c -o mysh

debug: mysh.c
	gcc mysh.c -o mysh -DDEBUG=1 -ggdb3

clean:
	rm -rf mysh