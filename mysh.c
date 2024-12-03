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

const char* PATH[] = {"/usr/local/bin", "/usr/bin", "/bin", "/"};
int QUIT = 0;

typedef struct {
    char** args;    // args[0] = command name
    int argc;
} command;

char** read_sfile() {
    // Memory allocation b/c Valgrind errors
    char buf[BUFSIZE];
    char* line = malloc(sizeof(char*) * BUFSIZE);
    char** lines = malloc(sizeof(char*) * BUFSIZE);
    memset(buf, 0, sizeof(char) * BUFSIZE);
    memset(line, 0, sizeof(char*) * BUFSIZE);
    memset(lines, 0, sizeof(char*) * BUFSIZE);

    int bytes = 0;
    int pos = 0;
    int line_index = 0;

    while ((bytes = read(STDIN_FILENO, buf, BUFSIZE)) > 0) {
        for (int i = 0; i <= bytes; i++) {
            if (buf[i] == '\n' || buf[i] == '\t' || i == bytes) {
                // Null-terminate the line
                line[pos] = '\0';

                lines[line_index] = strdup(line);
                line_index++;
                
                pos = 0;
                memset(line,0,strlen(line));
            } else {
                line[pos] = buf[i];
                pos++;
            }
        }
    }
    free(line);
    return lines;
}

// Read lines from files
char** read_file(char* file) {
    char buf[BUFSIZE];
    char* line = malloc(sizeof(char*) * BUFSIZE);
    char** lines = malloc(sizeof(char*) * BUFSIZE);
    memset(buf, 0, sizeof(char) * BUFSIZE);
    memset(line, 0, sizeof(char*) * BUFSIZE);
    memset(lines, 0, sizeof(char*) * BUFSIZE);

    int bytes = 0;
    int pos = 0;
    int line_index = 0;

    int fd = open(file, O_RDONLY);

    if (!fd) {
        perror("Error opening input file");
        return NULL;
    }

    while ((bytes = read(fd, buf, BUFSIZE)) > 0) {
        for (int i = 0; i <= bytes; i++) {
            if (buf[i] == '\n' || i == bytes) {
                // Null-terminate the line
                line[pos] = '\0';

                lines[line_index] = strdup(line);
                line_index++;
                
                pos = 0;
                memset(line,0,strlen(line));
            } else {
                line[pos] = buf[i];
                pos++;
            }
        }
    }
    free(line);
    return lines;
}

// Read line from standard input
char* read_line() {
    char buf[BUFSIZE];
    int w_index = 0;    // Index in the current word
    int pos = 0;
    
    char* line = malloc(sizeof(char) * BUFSIZE);

    while (read(STDIN_FILENO, &buf[pos], 1) > 0) {
        // Check for newline
        if (buf[pos] == '\n') {
            // Check if newline is at end of word
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

// Parse command into command struct
command parse_cmd(char* line) {
    command cmd;
    cmd.args = malloc(sizeof(char*) * BUFSIZE);
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

// Get path of executable in PATH
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
void cd_cmd(command cmd);
void pwd_cmd(command cmd);
void which_cmd(command cmd);
void exit_cmd(command cmd);
void external_cmd(command cmd);

void cd_cmd(command cmd) {
    // Expects 1 arg (path to directory)
    // Use chdir()
    // Print error if given the wrong number of arguments
    if (cmd.argc < 2) {
        fprintf(stderr, "cd: too few arguments\n");
        return;
    } else if (cmd.argc > 2) {
        fprintf(stderr, "cd: too many arguments\n");
        return;
    }

    if(chdir(cmd.args[1])){
        // Error statement
        fprintf(stderr, "cd: no such file or directory: %s\n", cmd.args[1]);
    }
}

void pwd_cmd(command cmd) {
    // Expects no args
    // Use getcwd()
    if (cmd.argc != 1) {
        fprintf(stderr, "pwd: too many arguments\n");
        return;
    }
        
    char pwd[512];
    getcwd(pwd, sizeof(pwd));
    printf("%s\n",pwd);
}

void which_cmd(command cmd) {
    if (cmd.argc < 2) {
        fprintf(stderr, "which: too few arguments\n");
        return;
    } else if (cmd.argc > 2) {
        fprintf(stderr, "which: too many arguments\n");
        return;
    }

    // Argument cant be name of internal cmd
    if (!strcmp(cmd.args[1], "which") || !strcmp(cmd.args[1], "cd")
        || !strcmp(cmd.args[1], "pwd") || !strcmp(cmd.args[1], "exit"))
            return;

    char* path = get_path(cmd.args[1]);
    if (path)
        printf("%s\n", path);
    free(path);
}

void exit_cmd(command cmd) {
    if (cmd.argc > 1) {
        printf("exiting with args:");
        for (int i = 1; i < cmd.argc; i++) {
            printf(" %s", cmd.args[i]);
        }
    } else printf("exiting...");
    QUIT = 1;
}

void external_cmd(command cmd) {
    char* path = get_path(cmd.args[0]);
    if (!path) {
        fprintf(stderr, "command not found: %s\n", cmd.args[0]);
        return;
    }
    
    int stat;
    pid_t pid = fork();
    if (pid == 0) {
        execv(path, cmd.args);
        perror("execv");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        wait(&stat);
    } else {
        perror("fork");
    }

    if (WIFEXITED(stat) && WSTOPSIG(stat) != 0)
        fprintf(stderr, "mysh: command failed with code %d\n", WSTOPSIG(stat));
    free(path);
}


// COMMAND PROCESSORS
int check_rwp(command* cmd);
void redirect_input();
void redirect_output();
void eval_wildcard();
void eval_pipeline();
void eval_cmd();

// Check for special symbols (redirections, wildcards, pipes)
int check_rwp(command* cmd) {
    for (int i = 0; i < cmd->argc; i++) {
        if (strchr(cmd->args[i], '*')) {
            if (DEBUG) printf("check_rwp: wildcard detected\n");
            eval_wildcard(cmd);
        }

        if (strcmp(cmd->args[i], "|") == 0) {
            if (DEBUG) printf("check_rwp: pipe detected\n");
            return 1;
        }
        
        if (strchr(cmd->args[i], '>')) {
            if (DEBUG) printf("check_rwp: output redirection detected\n");
            return 2;
        }
        
        if (strchr(cmd->args[i], '<')) {
            if (DEBUG) printf("check_rwp: input redirection detected\n");
            return 3;
        }
    }
    return 0;
}

// Process input redirection
void redirect_input(command cmd) {
    int redirect_pos = -1;
    char* input_file;

    // Get location of '>' in args
    for (int i = 0; i < cmd.argc; i++) {
        if (strcmp(cmd.args[i], "<") == 0) {
            input_file = strdup(cmd.args[i + 1]);
            redirect_pos = i;

            // Remove output file from args and symbol
            cmd.args[i + 1] = NULL;
            cmd.args[i] = NULL;
            cmd.argc = cmd.argc - 2;
        } else if (strchr(cmd.args[i], '<')) {
            // strtok() modifies original input
            char* temp = strdup(cmd.args[i]);
            char* token = strtok(temp, "<");

            // Simplify expression to first argument
            cmd.args[i] = strdup(token);

            // Get 2nd argument in redirect expression
            token = strtok(NULL, "<");
            input_file = malloc(sizeof(char) * strlen(token));

            // Initialize var for memory safety
            memset(input_file, 0, strlen(token));
            input_file = strdup(token);

            free(temp);
        }
    }

    printf("input_file: %s\n", input_file);

    pid_t pid = fork();
    int status;

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        int fd = open(input_file, O_RDONLY);
        if (fd < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        dup2(fd, STDIN_FILENO);
        close(fd);

        char** input = read_sfile();
        for (int i = 0; input[i]; i++) {
            command temp = parse_cmd(input[i]);
        
            // Add redirected args to command args
            for (int j = 0; j < temp.argc; j++) {
                // free before allocating new
                free(cmd.args[cmd.argc + j]);
                printf("temp arg: %s\n", temp.args[j]);
                cmd.args[cmd.argc + j] = strdup(temp.args[j]);
                free(temp.args[j]);
            }
            cmd.argc = cmd.argc + temp.argc;
            
            free(temp.args);
            free(input[i]);
        }

        for (int i = 0; i < cmd.argc; i++) {
            printf("arg: %s\n", cmd.args[i]);
        }

        eval_cmd(cmd);
        exit(EXIT_SUCCESS);
    }

    waitpid(pid, &status, 0);
}

// Process output redirection
void redirect_output(command cmd) {
    int redirect_pos = -1;
    char* output_file;

    // Get location of '>' in args
    for (int i = 0; i < cmd.argc; i++) {
        if (strcmp(cmd.args[i], ">") == 0) {
            output_file = strdup(cmd.args[i + 1]);
            redirect_pos = i;

            // Remove output file from args and symbol
            cmd.args[i + 1] = NULL;
            cmd.args[i] = NULL;
            cmd.argc = cmd.argc - 2;
        } else if (strchr(cmd.args[i], '>')) {
            // strtok() modifies original input
            char* temp = strdup(cmd.args[i]);
            char* token = strtok(temp, ">");

            // Simplify expression to first argument
            cmd.args[i] = strdup(token);

            // Get 2nd argument in redirect expression
            token = strtok(NULL, ">");
            output_file = malloc(sizeof(char) * strlen(token));

            // Initialize var for memory safety
            memset(output_file, 0, strlen(token));
            output_file = strdup(token);

            free(temp);
        }
    }

    if (!output_file) {
        fprintf(stderr, "redirect_output: Missing output file for redirection\n");
        return;
    }

    pid_t pid = fork();
    int status;

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process
        int fd = open(output_file, O_RDWR | O_CREAT | O_TRUNC, 0640);
        if (fd < 0) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        dup2(fd, STDOUT_FILENO);
        close(fd);
        eval_cmd(cmd);
        exit(EXIT_SUCCESS);
    }

    waitpid(pid, &status, 0);

    free(output_file);
}

// Evaluate wildcards
void eval_wildcard(command* cmd) {
    // Temporary storage for new arguments
    char** new_args = malloc(sizeof(char*) * BUFSIZE);

    int new_argc = 0;
    glob_t gstruct;

    for (int i = 0; i < cmd->argc; i++) {
        if (strchr(cmd->args[i], '*')) {
            int r = glob(cmd->args[i], GLOB_ERR | GLOB_TILDE, NULL, &gstruct);
            if (r != 0) {
                printf("wildcard '%s' has no matches\n", cmd->args[i]);
                new_args[new_argc++] = strdup(cmd->args[i]);
                continue;
            }

            for (size_t j = 0; j < gstruct.gl_pathc; j++) {
                new_args[new_argc++] = strdup(gstruct.gl_pathv[j]);
            }
        } else {
            new_args[new_argc++] = strdup(cmd->args[i]);
        }
    }

    globfree(&gstruct);

    // Update command struct
    cmd->args = new_args;
    cmd->argc = new_argc;
    cmd->args[cmd->argc] = NULL; // Null-terminate the array
}


// Evaluate pipeline expressions
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
    lhs.args = malloc(sizeof(char*) * BUFSIZE);
    memset(lhs.args, 0, sizeof(char*) * BUFSIZE);

    for (lhs.argc = 0; lhs.argc < pipe_pos; lhs.argc++) {
        lhs.args[lhs.argc] = strdup(cmd.args[lhs.argc]);
        if (DEBUG) printf("lhs.args[%d]: %s\n", lhs.argc, lhs.args[lhs.argc]);
    }
    lhs.args[lhs.argc] = NULL;

    command rhs;
    rhs.args = malloc(sizeof(char*) * BUFSIZE);
    memset(rhs.args, 0, sizeof(char*) * BUFSIZE);

    for (rhs.argc = 0; rhs.argc < cmd.argc - (pipe_pos + 1); rhs.argc++) {
        rhs.args[rhs.argc] = strdup(cmd.args[pipe_pos + rhs.argc + 1]);
        if (DEBUG) printf("rhs.args[%d]: %s\n", rhs.argc, rhs.args[rhs.argc]);
    }
    rhs.args[rhs.argc] = NULL;
    
    // Create pipe
        // fd[0] is read end
        // fd[1] is write end
    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int lhs_status, rhs_status;

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) {
        // LHS child process
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        eval_cmd(lhs);
        exit(EXIT_SUCCESS);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid2 == 0) {
        // RHS child process
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);

        char** input = read_sfile();
        for (int i = 0; input[i]; i++) {
            command temp = parse_cmd(input[i]);
            
            // Add piped args to command args
            for (int j = 0; j < temp.argc; j++) {
                // free before allocating new
                free(rhs.args[rhs.argc + j]);
                rhs.args[rhs.argc + j] = strdup(temp.args[j]);
                free(temp.args[j]);
            }
            // Adjust RHS argc
            rhs.argc = rhs.argc + temp.argc;
            
            free(temp.args);
            free(input[i]);
        }
        free(input);
        eval_cmd(rhs);
        exit(EXIT_SUCCESS);
    }

    close(fd[0]);
    close(fd[1]);

    waitpid(pid1, &lhs_status, 0);
    waitpid(pid2, &rhs_status, 0);

    for (int i = 0; i < lhs.argc; i++) {
        free(lhs.args[i]);
    }
    free(lhs.args);

    for (int i = 0; i < rhs.argc; i++) {
        free(rhs.args[i]);
    }
    free(rhs.args);
}

// Evaluate commands
void eval_cmd(command cmd) {
    int special = check_rwp(&cmd);

    if (special == 1) {
        eval_pipeline(cmd);
        return;
    } else if (special == 2) {
        redirect_output(cmd);
        return;
    } else if (special == 3) {
        redirect_input(cmd);
        return;
    }

    if (!strcmp(cmd.args[0], "cd")) {
        cd_cmd(cmd);
    } else if (!strcmp(cmd.args[0], "pwd")) {
        pwd_cmd(cmd);
    } else if (!strcmp(cmd.args[0], "which")) {
        which_cmd(cmd);
    } else if (!strcmp (cmd.args[0], "exit")) {
        exit_cmd(cmd);
    } else {
        external_cmd(cmd);
    }
}

int main(int argc, char* argv[]) {
    int interactive = isatty(STDIN_FILENO);

    if (argc == 1) {
        // INTERACTIVE
        if (interactive)
            printf("welcome to my shell!\n");
    } else if (argc > 1) {
        // BATCH
        interactive = 0;
    }
    
    // COMMAND LOOP
    while (!QUIT) {
        if (!interactive) {
            // Store all lines in an array
            char** input = read_file(argv[1]);
            int i = 0;

            // Execute lines
            while (input[i]) {
                command cmd = parse_cmd(input[i]);
                eval_cmd(cmd);
                free(input[i]);
                i++;

                for (int j = 0; j < cmd.argc; j++) {
                    free(cmd.args[j]);
                }
                free(cmd.args);
            }

            free(input);
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

        // Free up memory
        free(input);
        for (int i = 0; i < cmd.argc; i++) {
            free(cmd.args[i]);
        }
        free(cmd.args);
    }

    return EXIT_SUCCESS;
}