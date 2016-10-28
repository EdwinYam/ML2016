import random
from collections import Counter
import math
import copy
import csv
from sys import argv
from datetime import datetime
import numpy as np
from tempfile import TemporaryFile

modelname = argv[1]
modelname += ".npy"
testname = argv[2]
if (".csv" not in testname):
	testname += ".csv"
predictfile = argv[3]

def csv_read(flag,filename):
	with open(filename, 'rb') as csvfile:
		for row in csv.reader(csvfile, delimiter=','):
			if flag == 2:
				csvdata = [row]
			elif flag > 2:
				csvdata.append(row)
			flag = flag + 1;
	csvfile.close()
	return csvdata

def classify_jungle(DAG, item):
    branch = DAG[0]
    while branch[0] is not None:
        try:
            feat, thre, L1, L2 = branch
            if (feat==True) or (item[feat]>thre):
                branch = DAG[L1]
            else:
                branch = DAG[L2]
        except:
            print len(branch)
            raise
    return branch[2]

# read in testset from csvfile
start = 1
flag = 2
csvtest = csv_read(flag,testname)
m = len(csvtest)
n = len(csvtest[0])
csvtest = [ [float(csvtest[x][y]) for y in xrange(start,n) ] for x in xrange(m) ]
test_number = len(csvtest)


jungle = np.load(modelname)


predict = []
x = 0
while (x < test_number):
	testSample = csvtest[x][:]
	########## To change
	c = Counter()
	for tree in jungle:
		c += classify_jungle(tree, testSample)
	if (c!=Counter()):
		predict += [max(c, key=lambda x: c[x])]
		x += 1


# generating prediction
with open(predictfile, 'w+') as csvfile:
	spamwriter = csv.writer(csvfile)
	spamwriter.writerow(["id"] + ["label"])
	for x in xrange(test_number):
		spamwriter.writerow(["%d"%(x+1)] + ["%d"%(predict[x])])