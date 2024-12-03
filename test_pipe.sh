echo --- PIPE FUNCTIONALITY TESTS ---
cd test_cases
echo creating create1.txt and create2.txt using pipe functions...
touch create1.txt | touch create2.txt
echo removing create1.txt and create2.txt...
rm create1.txt | rm create2.txt
echo creating files again...
touch create1.txt | touch create2.txt
cd ..
ls | wc
ls | grep test
exit