#! /bin/bash	

for i in tests/*.anna; do
    echo "Run test $i"
    ./anna tests/$(basename $i .anna) >anna_tests.out 2>/dev/null
    status=$?
    out_correct=tests/$(basename $i .anna).output
    status_correct=tests/$(basename $i .anna).status
    if test -f $out_correct; then
	if diff anna_tests.out $out_correct; then
	    true
	else
	    echo "Error in output for test $i!!"
	fi
    fi
    if test -f $status_correct; then
	if test $(cat $status_correct) = $status; then
	    true
	else
	    echo "Error in exit status for test $i." $(cat $status_correct) != $status
	fi
    fi
done