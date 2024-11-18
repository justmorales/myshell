#include <stdio.h>

#define BUFSIZE 1024



char* read_line() {}

char cmd_parser() {}



// BUILT-IN SHELL COMMANDS
void cd_cmd();
void pwd_cmd();
void which_cmd();

void cd_cmd() {
    // Expects 1 arg (path to directory)
    // Use chdir()
    // Print error if given the wrong number of arguments
}

void pwd_cmd() {
    // Expects no args
    // Use getcwd()
}

void which_cmd() {}