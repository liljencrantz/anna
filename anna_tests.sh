#! /bin/bash

error_count=0
test_count=0
for i in tests/*.anna; do
#    echo "Run test $i"
    ./anna tests/$(basename $i .anna) >anna_tests.out 2>/dev/null
    status=$?
    out_correct=tests/$(basename $i .anna).output
    status_correct_file=tests/$(basename $i .anna).status
    status_correct=0
    error=0
    if test -f $out_correct; then
	if diff anna_tests.out $out_correct >/dev/null; then
	    true
	else
	    error=1
	    echo -e "\nError in output for test $i:"
	    diff -u anna_tests.out $out_correct
	fi
    fi

    if test -f $status_correct_file; then
	status_correct=$(cat $status_correct_file)
    fi

    if test "$status" != "$status_correct"; then
	echo -e "\nError in exit status for test $i." $status_correct != $status
	error=1
    fi
    
    error_count=$(echo $error_count + $error|bc)
    test_count=$(echo $test_count + 1|bc)
    
done

rm anna_tests.out
echo -e "\nFound $error_count errors while running $test_count tests"
test $error_count = 0
