My Shell, CS214 Fall 2024 
jm2663 - Justin Morales
jpd247 - Justin De Jesus

Implentation of myshell used functions cd_cmd, pwd_cmd, which_cmd, external_cmd, eval_cmd, check_rwp, external_cmd, redirect_input, redirect_output, read_file, eval_wildcard,and eval_pipeline. Each of these functions helps run make up the commands fo the terminal, and how to read the input and interpret it, depending on the input, it will compare whether it is a redirect, wildcard, or pipe, then it will check for the command to run whether it is cd, pwd, which, exit or an external command. read_file function will read the file, then it will parse the the data into the struct of command. our global command struct holds the arguments of the input and the number of inputs. Also our other global variables path, an array of paths used as a frameowrk for our get_path function, and int quit, which will indicate when to stop running the program. We have our read_file and read_sfile, read_file will read the file passed in and take the tokens from the file passed in for it to parsed later and returns a pointer to an array of chars. read_sfile reads lines from standard input, handles each line or tab-separated entry, storing them in a dynamically allocated array and returns a pointer to an array of strings. The parse function will take in a pointer to some string and will chop it up by the white space and add it all into the command struct. Our eval_cmd will take in our command struct and check what command to run whether it be our internal or external commands and then it will run the intended function or if there is a redirect, wildcard or pipe, it will run that. Our get_path function, that will get the path by comparing it to our global variable and seeing which one can run when we make the path. Then we have our cd, pwd, which, and external command functions. All of these functions will check for the respective amount of arguments and then run their command. So for cd_cmd, it will change the directory to the input, pwd_cmd will print the path to the directory you are working in, which will print the path to the file that was inputted, and external_cmd will fork and get the command to run. Seeing how redirect input and output work, they will take in an array of args and the count. They will read the command, then either read from the file or print to the file. In redirect_input, it looks for < to see where the command and file will be, and read the file then run the command on the contents inside the file. For redirect_output it does the same where it finds > to know the command and then where to print to. It will use dup2 to print out to the file. it will run the command first then take the output and print it out to the file. Eval_wildcard will take the globs to evaluate the wildcard symbol and then in the struct we will replace the wildcard with the evaluated pathnames, For eval_pipeline it will locate where the pipe is in the inputs, then split the commands, then creat the pipe and fork the processes. Finally it will add the piped args to the comman struct we have so we can run the commands. Eval_cmd will just evaluate which of these functions to run in the correct order, so that the other commands can work with the right amount of args. To it run constantly, we have a while loop and just set it to 0, when the user inputs exit, it will set the value to 1 to exit and stop the program. For multiple iterations of redirect, it will have a special case boolean and will set it to true, so that the output of the first redirect is put at the very end of the command struct args, then when it hits the second redirect, it will read the args outputted from the first redirect at the new place where the new data was added. This is only a case for when there is multiple redirects in the input. At the very begining it will check if it is in interactive or batch mode by how many arguments is taken in at the very begining. If in batch mode, it will read the file that was passed in and then parse everything to the command struct, then run the commands. parse_cmd will take in a char* then split up the arguments and add them into the command struct arg by arg and also increase the argc to fit the amount of arguments in there. At the very end of the code, it will free the global variables such as the command struct and its values inside and then return EXIT_SUCCESS.

Test Cases
our test cases directory is to have some files to redirect input/output,pipe into and any other thing that deals with changing a file or taking in from or outputting to
<<<<<<< HEAD
=======

>>>>>>> ed33ab6 (final)
./mysh test_internal.sh 
will test the internal commands such as cd, pwd, which etc. echo, which is not an internal command is used to show which is being tested and also uses pwd to show that you cd is being used correctly

./mysh test_pipe.sh
will test if piping works correctly, it will navigate into the test case folder, then use touch to create a new file to pipe into and rm to remove files that were created, each line will show what is happening using echo

./mysh test_redirect.sh
will test if redirecting input and output work, it will cd into the test case folder and use echo to show what is happening, it will check what is happening in each file and show what is suppose to be in it

how to compile
MAKE TESTS
will run all of the tests we have above
to make an executable 
MAKE
to clean your executable and prep for making an executable
MAKE CLEAN
then run the code how you see fit, whether it will