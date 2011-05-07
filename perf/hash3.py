
d = {1:"baberiba", 2:"tjoho"}
d[4] = "flojt1"
d[20] = "flojt2"
print len(d)

print map(lambda x: x, d)

for i in xrange(0,100000):
    d[10*i] = "AAA"
print len(d)

for i in xrange(0,100000):
    del(d[10*i])
print len(d)

for i in xrange(0,100000):
    d[10*i] = "AAA"
print len(d)

for i in xrange(0,100000):
    del(d[10*i])
print len(d)

