# question 1
import sys
import numpy as np
cols = int(sys.argv[1])
filename = sys.argv[2]
data = open(filename,'r')
ans = open("ans2.txt",'w')
rows = [map(float,L.strip().split(' ')) for L in data]
arr = np.array(rows)
r, c = arr.shape

# np.set_printoptions(suppress=True)
sort = np.swapaxes(arr,0,1)
sort_ = sort[cols]
sort_ = np.sort(sort_,axis=None)
for i in xrange(sort_.size):
	if (i == sort_.size-1):
		ans.write('%g' % sort_[i])
	else:
		ans.write('%g' % sort_[i])
		ans.write(',')
