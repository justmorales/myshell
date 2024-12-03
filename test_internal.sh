echo --- INTERNAL COMMAND TESTS ---
echo
echo Directory should change from /myshell to /test_cases
pwd
cd test_cases
pwd
echo
echo Should output 2 file names
ls c*.txt
echo
echo Should output create1.txt
ls create1.txt
echo
cd ..
echo Going back to myshell directory
pwd
echo
echo Should output paths for echo and cat
which echo
which cat
echo
exit