# Naive implementation of the
# Sieve of Eratosthenes
import sys

end = int(sys.argv[1])
sieve = [None] * end
count = 0

for i in xrange(2, end):
    if sieve[i] is None:
	for j in xrange(2*i, end, i):
            sieve[j] = 1
        count+= 1

print "There are", count, "prime numbers under", end
