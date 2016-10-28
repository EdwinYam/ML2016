import random
from collections import Counter
import math
import copy
import csv
from sys import argv
from datetime import datetime
from tempfile import TemporaryFile
import numpy as np
random.seed(datetime.now())
dataname = argv[1]
if (".csv" not in dataname):
	dataname += ".csv"
modelname = argv[2]
def csv_read(flag,filename):
	with open(filename,'rb') as csvfile:
		csvdata = []
		for row in csv.reader(csvfile, delimiter=','):
			csvdata.append(row)
		row_number = len(csvdata)
		csvdata = [ csvdata[i] for i in range(flag,row_number) ]
	csvfile.close()
	return csvdata

flag = 0
start = 1
csvdata = csv_read(flag,dataname)
m = len(csvdata)
n = len(csvdata[0])
csvdata = [ [ float(csvdata[i][j]) for j in xrange(start,n) ] for i in xrange(m) ]

TOTAL_SIZE = len(csvdata)
DATA_FEATURE = 57
VALID_SIZE = 100
TRAIN_SIZE = TOTAL_SIZE - VALID_SIZE
NUMBER_OF_TREES = 10

def slice_valid(data,valid_size,total_size):
	data_size = len(data)
	valid_list = random.sample(xrange(0,data_size),valid_size)
	train_list = list(set(xrange(total_size)) - set(valid_list))
	validSet = [ data[x] for x in valid_list ]
	dataSet = [ data[x] for x in train_list ] 
	return validSet,dataSet
validSet,dataSet = slice_valid(csvdata,VALID_SIZE,TOTAL_SIZE)

validSet = [(data[:DATA_FEATURE],data[DATA_FEATURE]) for data in validSet ]
dataSet =  [(data[:DATA_FEATURE],data[DATA_FEATURE]) for data in dataSet ]

def entropy(data):
	v = Counter([b for _, b in data]).values()
	total_num = float(sum(v))
	d = [ v[i]/total_num for i in xrange(len(v))]
	return - sum([d[i]*math.log(d[i]) for i in xrange(len(d))])
	# d = np.array(v) / float(sum(v))
	# return - sum(d * np.log(d)) # Shonnan entropy

# Split data according to entropy, feature is int which representing the index 
def split(data, feature, thresold):  
	entropy_ = entropy(data)
	if entropy_ < 0.0000001:
		print "Entropy very low"
	 	raise Exception("Entropy very low")
	Left = []
	Right = []
	for sample in data:
		if (sample[0][feature] >= thresold):
			Left.append(sample)
		else:
			Right.append(sample)
	energyLeft = entropy(Left)
	energyRight = entropy(Right)
	dataSize = len(data)
	gain = entropy_ - energyLeft * len(Left)/float(dataSize) - energyRight * len(Right)/float(dataSize)
	return gain, Left, Right, feature

def find_threshold(data, feature): # feature is int which representing the index
	best_gain = 0.0
	best_threshold = None
	data_feature = [ data[i][0][feature] for i in xrange(len(data)) ]
	value_list = [ i for i in Counter(data_feature) ]
	for threshold in value_list:
		gain, _, _, _ = split(data, feature, threshold)
		if gain > best_gain:
			best_gain = gain
			best_threshold = threshold
	return best_gain, best_threshold

def find_split(data,features): # features is a list containing index of feat
	best_feature = None
	best_threshold = None
	best_gain = 0.0
	for feat in features:
		gain, threshold = find_threshold(data, feat)
		if gain > best_gain:
			best_gain = gain
			best_threshold = threshold
			best_feature = feat
	_, Left, Right, _ = split(data,best_feature,best_threshold)
	return best_feature, best_threshold, Left, Right

def build_tree(data, features, levels=12, numfeatures=30):
	if levels == 0:
		count = Counter([label for _, label in data])
		Leaf = (None, count)
		return Leaf
	else:
		try:
			feats = random.sample(features, numfeatures)
			best_feat, _, Left, Right = find_split(data,feats)
			L = build_tree(Left, features, levels - 1, numfeatures)
			R = build_tree(Right, features, levels - 1, numfeatures)
			Branch = (best_feat, best_threshold, L, R)
			return Branch
		except:
			return build_tree(data, features, levels=0)

def classify(tree, item):
	if len(tree) == 2:
		assert tree[0] is None
		return tree[1]
	else:
		feat, thre, Left, Right = tree
		if (item[feat]>=thre):
			return classify(Left, item)
		else:
			return classify(Right, item)

def build_jungle(data, features, levels=25, numfeatures=35):
    DAG = {0: copy.copy(data)} # shallow copy
    Candidate_sets = [0]
    next_ID = 0 # number of node
    node_limit = 20 # basic number of nodes

    for level in range(levels):
        result_sets = []
        for tdata_idx in Candidate_sets: 
            tdata = DAG[tdata_idx]
           	
            # determine behavior of the bottom leaf
            if entropy(tdata) == 0.0 or len(tdata) <= 2: 
                next_ID += 1 
                idx1 = next_ID
                result_sets += [idx1] 
                DAG[idx1] = tdata + []
                del DAG[tdata_idx][:]
                DAG[tdata_idx] += [True, None, idx1, idx1] # (feat,threshold,number of leaves,..)
                continue
            feats = random.sample(features, numfeatures)
            try:   
            	best_feature, best_threshold, Left, Right = find_split(tdata, feats)
            except:
            	print "tdata: ",tdata
            	C1 = Counter([b for _, b in tdata])
            	del DAG[tdata_idx][:]
            	DAG[tdata_idx] += [None, None, C1]
            	continue
            # Branch = (F, M1, M2)
            next_ID += 1
            idx1 = next_ID
            DAG[idx1] = Left
            next_ID += 1
            idx2 = next_ID
            DAG[idx2] = Right

            result_sets += [idx1, idx2]
            del DAG[tdata_idx][:]
            DAG[tdata_idx] += [best_feature, best_threshold, idx1, idx2]

        ## Now optimize the result sets here
        random.shuffle(result_sets)

        basic = result_sets[:node_limit]
        for r in result_sets[node_limit:]:
            maxv = None
            maxi = None
            for b in basic:
                L = float(len(DAG[r] + DAG[b]))
                e1 = len(DAG[r]) * entropy(DAG[r])
                e2 = len(DAG[b]) * entropy(DAG[b])
                newe = L * entropy(DAG[r] + DAG[b])
                score = abs(e1 + e2 - newe)
                if maxv is None:
                    maxv = score
                    maxi = b
                    continue
                if score < maxv:
                    maxv = score
                    maxi = b
            DAG[maxi] += DAG[r]
            del DAG[r]
            DAG[r] = DAG[maxi]

        Candidate_sets = basic

    for tdata_idx in Candidate_sets:
        tdata = DAG[tdata_idx]
        C1 = Counter([b for _, b in tdata])
        del DAG[tdata_idx][:]
        DAG[tdata_idx] += [None, None, C1]

    return DAG


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


train = list(dataSet)
test = list(validSet)
features = xrange(DATA_FEATURE)
print "preprocess done"

def _Jungle():
	manytrees = []
	jungle = []
	for i in range(NUMBER_OF_TREES-8):
		print "Build tree %s" % i
		size = len(train)*2 / 3
		training_sample = random.sample(train, size)

		tree = build_jungle(training_sample, features, numfeatures=35)
		jungle += [tree]
		'''
		tree = build_tree(training_sample, features, numfeatures=30)
		manytrees += [tree]
		'''
	return jungle
jungle = _Jungle()

testdata = test
results_tree = Counter()
results_jungle = Counter()
for sample, label in testdata:
    '''
    # Trees
    c = Counter()
    for tree in manytrees:
        c += classify(tree, sample)
    res = (max(c, key=lambda x: c[x]), label)
    results_tree.update([res])
    '''
    # Jungle
    c = Counter()
    for tree in jungle:
        c += classify_jungle(tree, sample)
    res = (max(c, key=lambda x: c[x]), label)
    results_jungle.update([res])
modelname += ".npy"
j = np.array(jungle)
np.save(modelname,j)


print
print "Results         Jungle"
print "True positives:  %4d"%results_jungle[(1, 1)]
print "True negatives:  %4d"%results_jungle[(0, 0)]
print "False positives: %4d"%results_jungle[(1, 0)]
print "False negatives: %4d"%results_jungle[(0, 1)]



