compile: mysh.c
	gcc mysh.c -o mysh

debug: mysh.c
	gcc mysh.c -o mysh -DDEBUG=1

clean:
	rm -rf mysh