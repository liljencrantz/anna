steps = 100000
m = {}
for offset in xrange(0, 100000,10000):
    for i in xrange(offset, steps+offset):
        m["string" + str(i)] = i
    print len(m)
    for i in xrange(offset, steps+offset):
        nam = "string" + str(i-50000)
        if nam in m:
            del m[nam]
    print len(m)
