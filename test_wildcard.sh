echo --- WILDCARD TESTING ---
echo
echo Should output all files with .sh extension
ls *.sh
echo
echo Should output all files with .sh extension and README.md
ls *.sh README.md
echo
echo Moving into /test_cases...
cd test_cases
pwd
echo
echo Should output all files with that start with 'c' and end with .txt
ls c*.txt
echo
exit