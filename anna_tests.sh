#! /bin/bash	

for i in tests/*.anna; do
    if diff <(./anna tests/$(basename $i .anna) ) tests/$(basename $i .anna).out >/dev/null 2>&1; then
	true
    else
	echo Error in test $i
    fi
done