import os
import csv
import math
import random
from tempfile import TemporaryFile
from datetime import datetime
from sys import argv
random.seed(datetime.now())
dataname = argv[1]
modelname = argv[2]
modelname += ".csv"
# csv data read in
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

flag = 2
start = 1 # effective column start from it
csvdata = csv_read(flag,dataname)
m = len(csvdata)
n = len(csvdata[0])
csvlabel = [ int(csvdata[x][n-1]) for x in xrange(m) ]
csvdata = [ [float(csvdata[x][y]) for y in xrange(start,n-1) ] for x in xrange(m) ]

# seprating data set and validation set
data_feature = 57
total_si = len(csvdata)
valid_si = 100 # the slice for validation set
data_si = total_si - valid_si # the slice for testset

def slicing_valid(rawData,rawLabel):	
	data_list = random.sample(xrange(0,len(rawLabel)), data_si)
	valid_list = list(set(xrange(len(rawLabel))) - set(data_list))
	Dataset = [[ rawData[x][y] for y in xrange(data_feature) ] for x in data_list ]
	DataLabel = [ rawLabel[x] for x in data_list ]
	Validset =  [[ rawData[x][y] for y in xrange(data_feature) ] for x in valid_list ]
	ValidLabel = [ rawLabel[x] for x in valid_list ]
	return Dataset,DataLabel,Validset,ValidLabel

Dataset,DataLabel,Validset,ValidLabel = slicing_valid(csvdata,csvlabel)

# data normalization
DataNorm = Dataset[:]
ValidNorm = Validset[:]
def zerolistmaker(m,n):
	if (m==1):
		listofzeros = [0.0] * n
	else:
		listofzeros = [[0.0] * n for i in xrange(m)]
	return listofzeros

def data_normalization(dat):
	data_size = len(dat);
	normalize = zerolistmaker(data_feature,2) # mean and variation
	for x in xrange(data_feature):
		for y in xrange(data_size):
			normalize[x][0] = normalize[x][0] + dat[y][x]
			normalize[x][1] = normalize[x][1] + dat[y][x]*dat[y][x]
		normalize[x][0] = normalize[x][0]/data_size
		normalize[x][1] = normalize[x][1]/data_size
		variation = abs(normalize[x][1]-normalize[x][0]*normalize[x][0])
		normalize[x][1] = math.sqrt(variation)
		if (normalize[x][1] == 0): 
			normalize[x][1] = 0.01

	for x in xrange(data_size):
		for y in xrange(data_feature):
				dat[x][y] = (dat[x][y] - normalize[y][0])/normalize[y][1]
	return normalize,dat
	
data_normalize,DataNorm = data_normalization(DataNorm)
_,ValidNorm = data_normalization(ValidNorm)


for x in xrange(data_si):
		for y in xrange(data_feature):
				DataNorm[x][y] = (DataNorm[x][y]*data_normalize[y][1])+data_normalize[y][0]

# transform dataset shape(transpose)
def Batchize(Set,Label,batchSize):  
	_set = []
	_label = []
	for x in xrange(0,len(Label)-batchSize,batchSize):
		_set.append([Set[i+x] for i in xrange(batchSize)])
		_label.append([Label[i+x] for i in xrange(batchSize)])
	return _set,_label
minibatchSize = 5
Dataset,DataLabel = Batchize(Dataset,DataLabel,minibatchSize)
# Validset,ValidLabel = Batchize(Validset,ValidLabel,minibatchSize)

##########################################################################
# weight and bias initialization
path = "./"
capacity = 1
csv_files = [f for f in os.listdir(path) if f.endswith(".csv")]
smooth = 0.00000005
if (modelname in csv_files) and ("weight_adagrad.csv" in csv_files):
	flag = 2
	start = 0
	weight = csv_read(flag,"weight_ada.csv")
	bias = float(weight[1][0])
	weight = [[ float(weight[y][x]) for x in xrange(data_feature) ] for y in xrange(len(weight)-1)]
	flag = 2
	start = 0
	weight_ada = csv_read(flag,"weight_adagrad.csv")
	bias_ada = float(weight_ada[1][0])
	weight_ada = [[ float(weight_ada[y][x]) for x in xrange(data_feature) ] for y in xrange(len(weight_ada)-1) ]
	
else:
	weight = [[ random.gauss(0, 1.0) for x in xrange(data_feature) ] for y in xrange(capacity)]
	bias = random.gauss(0, 0.001)
	weight_ada = [[smooth for x in xrange(data_feature)] for y in xrange(capacity)]
	bias_ada = 0.0 + smooth 

def sigmoid(gamma):
    if gamma < 0:
        return 1 - 1 / (1 + math.exp(gamma))
    return 1 / (1 + math.exp(-gamma))
# generating prediction
def feedForward(sample):
	z = (sum([a*b for a,b in zip(sample,weight[0])]) + bias)
	return sigmoid(z)

# validation test --> generating batch
def generating_valid(Validset,ValidLabel):
	co = 0.0
	for x in xrange(len(ValidLabel)):
		predict = feedForward(Validset[x])
		if (predict>=0.5): 
			predict = 1
		else:
			predict = 0
		if (predict==ValidLabel[x]):
			co += 1
	accuracy = co/len(ValidLabel)
	return accuracy

# set training condition
learning_rate = 0.01 
# reg = 10
epoch = 2500

# previous trained information loaded 
if ("history.csv" in csv_files):
	flag = 2
	start = 0
	info = csv_read(flag,"history.csv")
	info = [ [int(info[x][0]), float(info[x][1]) ] for x in xrange(len(info)) ]
else: 
	info = zerolistmaker(1,2)
	info = [info]
# training
# Dataset = Data_norm
for j in xrange(epoch):
	current_info = zerolistmaker(1,2) 
	# gradient descent
	for minibatch,minilabel in zip(Dataset,DataLabel):
		grad_w = zerolistmaker(1,data_feature)
		grad_b = 0.0
		for sample,label in zip(minibatch,minilabel):
			predict = feedForward(sample)
			# temp = 2*(predict - label)*predict*(1 - predict)
			temp = -1.0*(label-predict)
			grad_b = grad_b + temp/minibatchSize
			for y in xrange(data_feature):
				current_grad = temp*sample[y]/minibatchSize
				grad_w[y] = grad_w[y] + current_grad # + 2*reg*weight[x][y]

 		bias_ada = math.sqrt(pow(bias_ada,2)+pow(grad_b,2))
		# update new bias
		bias = bias - (learning_rate/bias_ada)*grad_b
		# updatw new weight
		for y in xrange(data_feature):
			weight_ada[0][y] = math.sqrt(pow(weight_ada[0][y],2)+pow(grad_w[y],2))
			weight[0][y] = weight[0][y] - learning_rate/weight_ada[0][y]*grad_w[y] 
	# test
	if (j%50==0):
		accuracy = 0.0
		count = 0.0
		for minibatch,minilabel in zip(Dataset,DataLabel):
			for sample,label in zip(minibatch,minilabel):
				predict = feedForward(sample)
				if (predict>=0.5): 
					predict = 1
				else:
					predict = 0
				if (predict==label):
					count += 1
		print "TestLoss = ",count/(data_si-data_si%5)
		ValidationLoss = generating_valid(Validset,ValidLabel)
		print "ValidationLoss = ",ValidationLoss
		current_info[1] = ValidationLoss
		current_info[0] = len(info) + 1
		info.append(current_info)

# save weight and bias in csv file

###############################################

# save weight and bias in csv file
def csv_save(filename, data):
	with open(filename, 'w+') as csvfile:
		spamwriter = csv.writer(csvfile)
		spamwriter.writerows(data)
	csvfile.close()
weight_ = list(weight)
weight_ada_ = list(weight_ada)
weight_.append([bias])
weight_ada_.append([bias_ada])
csv_save(modelname, weight_)
csv_save("weight_adagrad.csv", weight_ada_)

######################################################################################################
# generating training process history
filename = 'history.csv'
with open(filename, 'w+') as csvfile:
	spamwriter = csv.writer(csvfile)
	for x in xrange(len(info)):
		spamwriter.writerow(["%d"%(info[x][0])] + ["%f"%(info[x][1])])
