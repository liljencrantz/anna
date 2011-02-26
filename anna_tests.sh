#! /bin/bash	

for i in tests/*.anna; do
    echo "Run test $i"
    if diff <(./anna tests/$(basename $i .anna) ) tests/$(basename $i .anna).out; then
	true
    else
	echo "Error in test $i!!"
    fi
done