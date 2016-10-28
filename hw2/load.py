import os
import csv
import math
import random
from tempfile import TemporaryFile
from datetime import datetime
from sys import argv
modelname = argv[1]
modelname += ".csv"
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
	# read in testset from csvfile
start = 1
flag = 2
csvtest = csv_read(flag,testname)
m = len(csvtest)
n = len(csvtest[0])
csvtest = [ [float(csvtest[x][y]) for y in xrange(start,n) ] for x in xrange(m) ]
_,testNorm = data_normalization(csvtest)
test_number = len(csvtest)

data_feature = 57
flag = 2
start = 0
weight = csv_read(flag,modelname)
bias = float(weight[1][0])
weight = [[ float(weight[y][x]) for x in xrange(data_feature) ] for y in xrange(len(weight)-1)]


def sigmoid(gamma):
    if gamma < 0:
        return 1 - 1 / (1 + math.exp(gamma))
    return 1 / (1 + math.exp(-gamma))
# generating prediction
def feedForward(sample):
	z = (sum([a*b for a,b in zip(sample,weight[0])]) + bias)
	return sigmoid(z)



# generating prediction
with open(predictfile, 'w+') as csvfile:
	spamwriter = csv.writer(csvfile)
	spamwriter.writerow(["id"] + ["label"])
	count = 0.0
	for x in xrange(test_number):
		testSample = csvtest[x][:]

		########## To change
		predict = feedForward(testSample)
		if(predict>=0.5):
			predict = 1
		else:
			predict = 0
		##########
		spamwriter.writerow(["%d"%(x+1)] + ["%d"%(predict)])