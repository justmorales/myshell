echo --- REDIRECT TESTS ---
echo
echo Printing "WELCOME TO REDIR_TEST.TXT" to redir_test.txt
echo WELCOME TO THE REDIR_TEST.txt > redir_test.txt
echo
echo Printing contents of redir_test.txt
echo < redir_test.txt
echo
echo Hello World! > output.txt
echo output.txt file should contain only "Hello World!" 
echo
echo Clearing redir_test.txt
echo > redir_test.txt
echo
exit