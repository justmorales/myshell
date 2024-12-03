echo --- PIPE TESTS ---
echo
cd test_cases
echo Creating create1.txt and create2.txt using pipe functions
touch create1.txt | touch create2.txt   
echo Removing create1.txt and create2.txt
rm create1.txt | rm create2.txt
echo Creating files again...
touch create1.txt | touch create2.txt
cd ..
echo
echo All lines in all .sh files which contain "echo" should output
echo
ls *.sh | grep echo
echo
exit