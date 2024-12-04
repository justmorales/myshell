compile: mysh.c
	gcc mysh.c -o mysh

debug: mysh.c
	gcc mysh.c -o mysh -DDEBUG=1 -ggdb3

tests: mysh.c
	gcc mysh.c -o mysh
	clear
	./mysh test_internal.sh
	./mysh test_pipe.sh
	./mysh test_redirect.sh
	./mysh test_wildcard.sh

clean:
	rm -rf mysh