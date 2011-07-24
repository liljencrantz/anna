#! /bin/bash 

# Runs all regression tests in the tests/ directory, and checks their
# output and exit status. For every .anna file in tests/, if there is
# a corresponding .output file, it is assumed to contain the desired
# output of the script. If there is a corresponding .status file, it
# is assumed to contain the desired exit status. If no .status file
# exists, it is assumed that the test should have exit status 0.

exec 2> /dev/null

error_count=0
test_count=0

for i in tests/*.anna; do
    ./bin/anna tests/$(basename $i .anna) >anna_tests.out 2>/dev/null
    status=$?
    out_correct=tests/$(basename $i .anna).output
    status_correct_file=tests/$(basename $i .anna).status
    status_correct=0
    error=0

    if test -f $status_correct_file; then
	status_correct=$(cat $status_correct_file)
    fi

    if test "$status" != "$status_correct"; then
	echo -e "\n\nError in exit status for test $i." $status_correct != $status
	error=1
    else
	if test -f $out_correct; then
	    if diff anna_tests.out $out_correct >/dev/null; then
		true
	    else
		error=1
		echo -e "\n\nError in output for test $i:"
		diff -u anna_tests.out $out_correct
	    fi
	fi
    fi
        
    error_count=$(echo $error_count + $error|bc)
    test_count=$(echo $test_count + 1|bc)    
    echo -n .
done

rm anna_tests.out
echo -e "\n\nFound $error_count errors while running $test_count tests"
test $error_count = 0