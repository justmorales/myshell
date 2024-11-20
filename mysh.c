#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFSIZE 1024

#ifndef DEBUG
#define DEBUG 0
#endif

const char* PATH[] = {"/usr/local/bin", "/usr/bin", "/bin"};
int QUIT = 0;

typedef struct {
    char** args;
    int argc;
    char* command;
} command;

char* read_line() {
    char buf[BUFSIZE];
    int pos = 0;
    int bytes;

    while (bytes = read(STDIN_FILENO, &buf[pos], 1) > 0) {
        // Check for EOF or newline
        if (bytes == 0) break;
        if (buf[pos] == '\n') break;
        pos++;
    }
    // Null-terminate at end of string
    buf[pos] = '\0';

    char* line = malloc(sizeof(char) * pos);
    memcpy(line, buf, pos);

    return line;
}

command parse_cmd(char* line) {
    char* token = strtok(line, " ");

    command cmd;
    cmd.args = malloc(sizeof(char*));
    cmd.argc = 0;
    cmd.command = malloc(sizeof(char*));
    cmd.command = token;
    token = strtok(NULL, " ");


    while (token) {
        int len = strlen(token);
        
        cmd.args[cmd.argc] = malloc(len);
        strcpy(cmd.args[cmd.argc], token);
        
        cmd.argc++;
        token = strtok(NULL, " ");
    }

    return cmd;
}

char* get_path(char* string) {
    // 3 = # of directories in PATH
    for (int i = 0; i < 3; i++) {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", PATH[i], string);

        if (!access(full_path, F_OK) && !access(full_path, X_OK)) {
            char* temp = malloc(sizeof(char) * 512);
            memcpy(temp, full_path, 512);
            return temp;
        }
    }

    return NULL;
}

// SHELL COMMANDS
void cd_cmd();
void pwd_cmd();
void which_cmd();
void external_cmd();

void cd_cmd(char** args, int argc) {
    // Expects 1 arg (path to directory)
    // Use chdir()
    // Print error if given the wrong number of arguments
    if (argc < 1) {
        printf("cd: too few arguments\n");
        return;
    } else if (argc > 1) {
        printf("cd: too many arguments\n");
        return;
    }

    if(chdir(args[0])){
        // Error statement
        printf("cd: no such file or directory: %s\n", args[0]);
    }
}

void pwd_cmd(int argc) {
    // Expects no args
    // Use getcwd()
    if (argc != 0) {
        printf("pwd: too many arguments\n");
        return;
    }
        
    char pwd[512];
    getcwd(pwd, sizeof(pwd));
    printf("%s\n",pwd);
}

void which_cmd(char** args, int argc) {
    if (argc < 1) {
        printf("which: too few arguments");
        return;
    } else if (argc > 1) {
        printf("which: too many arguments");
        return;
    }

    // arg cant be name of internal cmd
    if (!strcmp(args[0], "which") || !strcmp(args[0], "cd")
        || !strcmp(args[0], "pwd") || !strcmp(args[0], "exit"))
            return;

    printf("%s\n", get_path(args[0]));
}

void external_cmd(char* cmd, char* args) {
    // char* path = get_path(cmd);
    // if (!path) return;
    // pid_t child = fork();
    // if (child == 0) {
    //     execv(path, );
    // }
}

int main(int argc, char* argv[]) {
    int interactive = isatty(STDIN_FILENO);
    int fd;

    if (argc == 1) {
        // INTERACTIVE / NO ARGS
        if (interactive)
            printf("Welcome to my shell!\n");
    } else if (argc > 1) {
        // BATCH
        fd = open(argv[1], O_RDONLY);
        interactive = 0;
        if (fd == -1) {
            printf("main: Error opening file: %s\n", argv[1]);
        }
    }
    
    // COMMAND LOOP
    char* input;

    while (!QUIT) {
        if (!interactive) {
            // execute command here
            break;
        } else {
            // Use fflush to force standard buffer to output to stdout immediately
            printf("mysh> ");
            fflush(stdout);
            input = read_line();
        }

        command cmd = parse_cmd(input);
        if (!strcmp(cmd.command, "cd")) {
            cd_cmd(cmd.args, cmd.argc);
        } else if (!strcmp(cmd.command, "pwd")) {
            pwd_cmd(cmd.argc);
        } else if (!strcmp(cmd.command, "which")) {
            which_cmd(cmd.args, cmd.argc);
        } else if (!strcmp (cmd.command, "exit")) {
            printf("exiting...\n");
            QUIT = 1;
        } else {
            // Reassemble string from arg array
            char* arg_string = malloc(sizeof(char) * 512);
            for (int i = 0; i < cmd.argc; i++) {
                strcat(arg_string, " ");
                strcat(arg_string, cmd.args[i]);
            }
            printf("arg_string: %s\n", arg_string);
            external_cmd(cmd.command, arg_string);
        }

        printf("\tARGC: %d\n", cmd.argc);
    }

    return EXIT_SUCCESS;
}