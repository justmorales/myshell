#include <fcntl.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFSIZE 1024

#ifndef DEBUG
#define DEBUG 0
#endif

const char* PATH[] = {"/usr/local/bin", "/usr/bin", "/bin"};
int QUIT = 0;

typedef struct {
    char** args;    // args[0] = command name
    int argc;
} command;

char** read_file(char* file) {
    char buf[BUFSIZE];
    char* line = malloc(sizeof(char*)*BUFSIZE);
    char** lines = malloc(sizeof(char*)*BUFSIZE);

    int pos = 0;
    int bytes = 0;
    int line_index = 0;

    int fd = open(file, O_RDONLY);

    if (!fd) {
        perror("Error opening input file");
        return NULL;
    }

    while ((bytes = read(fd, buf, BUFSIZE)) > 0) {
        for (int i = 0; i <= bytes; i++) {
            if (buf[i] == '\n' || i == bytes) {
                line[pos] == '\0';
                if (DEBUG) printf("line %d: %s\n", line_index, line);
                lines[line_index] = strdup(line);
                line_index++;
                pos = 0;
                // Reset line to 0s
                memset(line,0,strlen(line));
            } else {
                if (DEBUG) printf("line_index: %d\n", line_index);
                if (DEBUG) printf("\tpos: %d\n", pos);
                if (DEBUG) printf("\ti: %d\n", i);
                if (DEBUG) printf("\t\tchar: %c\n", buf[i]);
                line[pos] = buf[i];
                pos++;
            }
        }
    }

    return lines;
}

char* read_line() {
    char buf[BUFSIZE];
    int w_index = 0;    // index in the current word
    int pos = 0;
    
    char* line = malloc(sizeof(char)*BUFSIZE);

    while (read(STDIN_FILENO, &buf[pos], 1) > 0) {
        // Check for newline
        if (buf[pos] == '\n') {
            if (w_index > 1) {
                line[w_index] = '\0';
                w_index = 0;
                return line;
            }
        } else {
            line[w_index] = buf[pos];
            w_index++;
        }
        pos++;
    }

    // Null-terminate at end of string
    line[pos] = '\0';

    return line;
}

command parse_cmd(char* line) {
    command cmd;
    cmd.args = malloc(BUFSIZE * sizeof(char*));
    cmd.argc = 0;

    char* token = strtok(line, " \n");

    while (token) {
        cmd.args[cmd.argc] = strdup(token);
        cmd.argc++;
        token = strtok(NULL, " \n");
    }

    // Args array should terminate with null
    cmd.args[cmd.argc] = NULL;
    return cmd;
}

char* get_path(char* string) {
    // 3 = # of directories in PATH
    for (int i = 0; i < 3; i++) {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", PATH[i], string);

        if (access(full_path, X_OK) == 0) {
            return strdup(full_path);
        }
    }

    return NULL;
}

// SHELL COMMANDS
void cd_cmd();
void pwd_cmd();
void which_cmd();
void exit_cmd();
void external_cmd();
void eval_cmd();

void cd_cmd(char** args, int argc) {
    // Expects 1 arg (path to directory)
    // Use chdir()
    // Print error if given the wrong number of arguments
    if (argc < 2) {
        printf("cd: too few arguments\n");
        return;
    } else if (argc > 2) {
        printf("cd: too many arguments\n");
        return;
    }

    if(chdir(args[1])){
        // Error statement
        printf("cd: no such file or directory: %s\n", args[1]);
    }
}

void pwd_cmd(int argc) {
    // Expects no args
    // Use getcwd()
    if (argc != 1) {
        printf("pwd: too many arguments\n");
        return;
    }
        
    char pwd[512];
    getcwd(pwd, sizeof(pwd));
    printf("%s\n",pwd);
}

void which_cmd(char** args, int argc) {
    if (argc < 2) {
        printf("which: too few arguments");
        return;
    } else if (argc > 2) {
        printf("which: too many arguments");
        return;
    }

    // Argument cant be name of internal cmd
    if (!strcmp(args[1], "which") || !strcmp(args[1], "cd")
        || !strcmp(args[1], "pwd") || !strcmp(args[1], "exit"))
            return;

    char* path = get_path(args[1]);
    if (path)
        printf("%s\n", path);
    free(path);
}

void exit_cmd(char** args, int argc) {
    if (argc > 1) {
        printf("exiting with args:");
        for (int i = 1; i < argc; i++) {
            printf(" %s", args[i]);
        }
        printf("...");
    } else printf("exiting...");
    QUIT = 1;
}

void external_cmd(char* cmd, char** args) {
    char* path = get_path(cmd);
    if (!path) {
        fprintf(stderr, "Command not found: %s\n", cmd);
        return;
    }
    
    int stat;
    pid_t pid = fork();
    if (pid == 0) {
        execv(path, args);
        perror("execv");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        wait(&stat);
    } else {
        perror("fork");
    }

    if (WIFEXITED(stat) && WSTOPSIG(stat) != 0)
        printf("mysh: command failed with code %d\n", WSTOPSIG(stat));
    free(path);
}

// check for special symbols (redirection, pipes, wildcards)
int check_rwp(char** args, int argc) {
    for (int i = 0; i < argc; i++) {
        if (strchr(args[i], '*')) {
            if (DEBUG) printf("Wildcard detected\n");
            return 1;
        } else if (strcmp(args[i], "|") == 0) {
            if (DEBUG) printf("Pipe detected\n");
            return 2;
        } else if (strcmp(args[i], "<") == 0) {
            if (DEBUG) printf("Input redirection detected\n");
            return 3;
        } else if (strcmp(args[i], ">") == 0) {
            if (DEBUG) printf("Output redirection detected\n");
            return 4;
        }
    }
    return 0;
}

void redirect_input(command* cmd) {
    int redirect_pos = -1;

    // Locate '<' in arguments
    for (int i = 0; i < cmd->argc; i++) {
        if (strcmp(cmd->args[i], "<") == 0) {
            redirect_pos = i;
            break;
        }
    }

    if (redirect_pos == -1 || redirect_pos + 1 >= cmd->argc) {
        fprintf(stderr, "Error: Missing input file for redirection\n");
        return;
    }
    
    char* input_file = cmd->args[redirect_pos + 1];

    // Remove '<' and input file from original arguments
    for (int i = 0; i < 2; i++) {
        cmd->args[cmd->argc] = NULL;
        cmd->argc--;
    }

    char* input = read_file(input_file)[0];
    command temp = parse_cmd(input);

    // Read file contents and add to command arguments
    for (int i = 0; i < temp.argc; i++) {
        cmd->args[cmd->argc] = temp.args[i];
        cmd->argc++;
    }

    // Null-terminate the arguments array
    cmd->args[cmd->argc] = NULL;

    // Execute the updated command
    eval_cmd(*cmd);
}

void redirect_output(char** args, int argc) {
    int redirect_pos = -1;
    for (int i = 0; i < argc; i++) {
        if (strcmp(args[i], ">") == 0) {
            redirect_pos = i;
            break;
        }
    }

    if (redirect_pos == -1 || redirect_pos + 1 >= argc) {
        fprintf(stderr, "Error: Missing output file for redirection\n");
        return;
    }
    char* output_file = args[redirect_pos + 1];
    args[redirect_pos] = NULL;

    pid_t pid = fork();
    if (pid == 0) {
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("Error opening output file");
            exit(EXIT_FAILURE);
        }
        // print to file
        dup2(fd, STDOUT_FILENO);
        close(fd);
        execvp(args[0], args);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        wait(NULL);
    } else {
        perror("fork");
    }
}

void eval_wildcard(command* cmd) {
    // cmd is pass by reference
    char* wildcard;
    int wc_pos;

    // Locate wildcard in args
    for (int i = 0; i < cmd->argc; i++) {
        if (strchr(cmd->args[i], '*')) {
            wildcard = cmd->args[i];
            wc_pos = i;
        }
    }

    char** found;
    glob_t gstruct;
    int r;

    r = glob(wildcard, GLOB_ERR, NULL, &gstruct);

    if (r != 0) {
        printf("wildcard: no matches");
        return;
    }

    // Store matched pathnames in array
    found = gstruct.gl_pathv;

    // -1 to replace wildcard
    cmd->argc--;

    // Replace wildcard with evaluated pathnames
    while (*found) {
        cmd->args[cmd->argc] = strdup(*found);
        cmd->argc++;
        found++;
    }

    globfree(&gstruct);
}

void eval_pipeline(command cmd) {
    int pipe_pos = 0;
    
    // Locate '|'
    for (int i = 0; i < cmd.argc; i++) {
        if (strcmp(cmd.args[i], "|") == 0) {
            pipe_pos = i;
        }
    }

    // Split commands
    command lhs;
    lhs.argc = 0;
    lhs.args = malloc(BUFSIZE * sizeof(char*));

    for (lhs.argc = 0; lhs.argc < pipe_pos; lhs.argc++) {
        lhs.args[lhs.argc] = cmd.args[lhs.argc];
        if (DEBUG) printf("lhs.args[%d]: %s\n", lhs.argc, lhs.args[lhs.argc]);
    }

    command rhs;
    rhs.argc = 0;
    rhs.args = malloc(BUFSIZE * sizeof(char*));

    for (rhs.argc = 0; rhs.argc < cmd.argc - (pipe_pos + 1); rhs.argc++) {
        rhs.args[rhs.argc] = cmd.args[pipe_pos + rhs.argc + 1];
        if (DEBUG) printf("rhs.args[%d]: %s\n", rhs.argc, rhs.args[rhs.argc]);
    }
    
    // Create pipe
        // fd[0] is read end
        // fd[1] is write end
    int fd[2];

    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int lhs_status;
    int rhs_status;

    pid_t pid1 = fork();

    if (pid1 == 0) {
        // Child process
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        eval_cmd(lhs);

        exit(EXIT_SUCCESS);
    } else if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        pid_t pid2 = fork();
        
        if (pid2 == 0) {
            // Child process
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);

            char* input = read_line();
            command temp = parse_cmd(input);

            // Add piped args to command args
            for (int i = 0; i < temp.argc; i++) {
                rhs.args[rhs.argc + i] = temp.args[i];
                rhs.argc++;
            }
            eval_cmd(rhs);

            free(input);
            for (int i = 0; i < rhs.argc; i++) {
                free(rhs.args[i]);
            }
            free(rhs.args);

            for (int i = 0; i < lhs.argc; i++) {
                free(lhs.args[i]);
            }
            free(lhs.args);

            exit(EXIT_SUCCESS);
        } else if (pid2 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else {
            close(fd[0]);
            close(fd[1]);

            waitpid(pid1, &lhs_status, 0);
            waitpid(pid2, &rhs_status, 0);
        }
    }
}

void eval_cmd(command cmd) {
    int special = check_rwp(cmd.args, cmd.argc);
    if (special == 2) {
        eval_pipeline(cmd);
        return;
    } else if (special == 1) {
        eval_wildcard(&cmd);
    } else if (special == 3) {
        redirect_input(&cmd);
        return;
    } else if (special == 4) {
        redirect_output(cmd.args, cmd.argc);
        return;
    }

    if (!strcmp(cmd.args[0], "cd")) {
        cd_cmd(cmd.args, cmd.argc);
    } else if (!strcmp(cmd.args[0], "pwd")) {
        pwd_cmd(cmd.argc);
    } else if (!strcmp(cmd.args[0], "which")) {
        which_cmd(cmd.args, cmd.argc);
    } else if (!strcmp (cmd.args[0], "exit")) {
        exit_cmd(cmd.args, cmd.argc);
    } else {
        external_cmd(cmd.args[0], cmd.args);
    }
}

int main(int argc, char* argv[]) {
    int interactive = isatty(STDIN_FILENO);

    if (argc == 1) {
        // INTERACTIVE / NO ARGS
        if (interactive)
            printf("welcome to my shell!\n");
    } else if (argc > 1) {
        // BATCH
        interactive = 0;
    }
    
    // COMMAND LOOP
    while (!QUIT) {
        if (!interactive) {
            char** input = read_file(argv[1]);
            int i = 0;
            while (input[i]) {
                command cmd = parse_cmd(input[i]);
                eval_cmd(cmd);
                i++;
            }
            break;
        } else {
            // Use fflush to force standard buffer to output to stdout immediately
            printf("mysh> ");
            fflush(stdout);
        }

        char* input = read_line();
        
        // Check if input is path to program
        if (input[0] == '/') {
            printf("path to program\n");
            continue;
        }

        command cmd = parse_cmd(input);
        eval_cmd(cmd);

        free(input);
        for (int i = 0; i < cmd.argc; i++) {
            free(cmd.args[i]);
        }
        free(cmd.args);
    }

    return EXIT_SUCCESS;
}