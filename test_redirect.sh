echo --- REDIRECT FUNCTIONALITY TESTING ---
cd test_cases
pwd
cd < wc_test.txt
pwd
echo directory should have changed to /myshell
echo Hello World! > output.txt
echo output.txt file in /test_cases should contain only "Hello World!" 
echo < wc_test.txt > output.txt
echo check output.txt again to see if it contains lines of "test"
exit