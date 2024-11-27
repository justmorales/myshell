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

char* read_line() {
    char buf[BUFSIZE];
    int pos = 0;

    while (read(STDIN_FILENO, &buf[pos], 1) > 0) {
        // Check for newline
        if (buf[pos] == '\n') break;
        pos++;
    }
    // Null-terminate at end of string
    buf[pos] = '\0';

    char* line = malloc(sizeof(char) * (pos + 1));
    strcpy(line, buf);

    return line;
}

command parse_cmd(char* line) {
    command cmd;
    cmd.args = malloc(BUFSIZE * sizeof(char*));
    cmd.argc = 0;

    char* token = strtok(line, " ");

    while (token) {
        cmd.args[cmd.argc] = strdup(token);
        cmd.argc++;
        token = strtok(NULL, " ");
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

void external_cmd(char* cmd, char** args) {
    char* path = get_path(cmd);
    if (!path) {
        fprintf(stderr, "Command not found: %s\n", cmd);
        return;
    }
    pid_t pid = fork();
    if (pid == 0) {
        execv(path, args);
        perror("execv");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        wait(NULL);
    } else {
        perror("fork");
    }
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

void redirect_input(char** args, int argc) {
    int redirect_pos = -1;

    // Locating < 
    for (int i = 0; i < argc; i++) {
        if (strcmp(args[i], "<") == 0) {
            redirect_pos = i;
            break;
        }
    }

    if (redirect_pos == -1 || redirect_pos + 1 >= argc) {
        fprintf(stderr, "Error: Missing input file for redirection\n");
        return;
    }

    // getting commmand and file
    char* cmd = args[0];
    char** cmd_args = malloc((redirect_pos + 1) * sizeof(char*));
    for (int i = 0; i < argc; i++) {
        cmd_args[i] = args[redirect_pos + i];
    }

    char* input_file = args[redirect_pos + 1];

    // reading file
    FILE* file = fopen(input_file, "r");
    if (!file) {
        perror("Error opening input file");
        free(cmd_args);
        return;
    }

    // Fork the process to execute the command
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        int fd = fileno(file);
        if (fd < 0) {
            perror("Error obtaining file descriptor");
            fclose(file);
            free(cmd_args);
            exit(EXIT_FAILURE);
        }

        // Redirect STDIN to the file
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("Error duplicating file descriptor");
            fclose(file);
            free(cmd_args);
            exit(EXIT_FAILURE);
        }

        fclose(file);

        // Execute the command with the redirected input
        execvp(args[0], cmd_args);
        perror("Error executing command");
        free(cmd_args);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process
        wait(NULL); // Wait for the child process to finish
    } else {
        perror("Error forking process");
    }

    fclose(file);
    free(cmd_args);
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
        eval_cmd(lhs);
        close(fd[1]);

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

            char* input = read_line();
            command temp = parse_cmd(input);

            // Add piped args to command args
            for (int i = 0; i < temp.argc; i++) {
                rhs.args[rhs.argc + i] = temp.args[i];
            }

            eval_cmd(rhs);
            close(fd[0]);

            free(input);
            for (int i = 0; i < rhs.argc; i++) {
                free(rhs.args[i]);
            }
            free(rhs.args);

            exit(EXIT_SUCCESS);
        } else if (pid2 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else {
            close(fd[0]);
            close(fd[1]);

            waitpid(pid1, &lhs_status, WUNTRACED);
            waitpid(pid2, &rhs_status, WUNTRACED);

            for (int i = 0; i < lhs.argc; i++) {
                free(lhs.args[i]);
            }
            free(lhs.args);

            for (int i = 0; i < rhs.argc; i++) {
                free(rhs.args[i]);
            }
            free(rhs.args);
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
        redirect_input(cmd.args, cmd.argc);
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
        printf("exiting...\n");
        QUIT = 1;
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
        freopen(argv[1], "r", stdin);
    }
    
    // COMMAND LOOP
    while (!QUIT) {
        if (!interactive) {
            printf("BATCH MODE");
            break;
        } else {
            // Use fflush to force standard buffer to output to stdout immediately
            printf("mysh> ");
            fflush(stdout);
        }

        char* input = read_line();
        if (!input) break;

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