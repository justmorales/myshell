echo --- INTERNAL COMMAND TESTS ---
pwd
cd test_cases
pwd
echo directory should have changed successfully from myshell to myshell/test_cases...
ls c*.txt
echo should print out 2 file names...
ls create1.txt
echo should only print create1.txt...
cd ..
pwd
echo going back to myshell directory...
cd ..
which echo
which cat
echo should print out locations for echo and cat...
exit