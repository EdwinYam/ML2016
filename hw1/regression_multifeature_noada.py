import os
import csv
import numpy as np
import math
import random
from tempfile import TemporaryFile
from datetime import datetime
random.seed(datetime.now())

# csv data read in

def csv_read(start,flag,filename):
	with open(filename, 'rb') as csvfile:
		for row in csv.reader(csvfile, delimiter=','):
			if flag == 2:
				csvdata = np.array((row))
			elif flag > 2:
				csvdata = np.vstack((csvdata,np.array(row)))
			flag = flag + 1;
	csvfile.close()
	csvdata = np.array([ csvdata[i][start:] for i in xrange(csvdata.shape[0]) ])
	for x in xrange(csvdata.shape[0]):
		for y in xrange(csvdata.shape[1]):
			if (csvdata[x][y] == 'NR'):
				csvdata[x][y] = '0'
	return csvdata.astype(float)

filename = 'train.csv'
flag = 1
start = 3 # effective column start from it
csvdata = csv_read(3,flag,filename)
# seprating data set and validation set
time = 9 
data_time = 24
data_feature = 18
data_si = 15 # the slice for testset per month
valid_si = 5 # the slice for validation set per month
total_si = 20

def slicing_valid(rawData):	
	data_list = []
	valid_list = []
	for x in xrange(rawData.shape[0]):
		if (((x/data_feature)/12)%total_si >= data_si) and (((x/data_feature)/12)%total_si < total_si):
			valid_list.append(x)
		else:	
			data_list.append(x)
	Dataset = [[ rawData[x][y] for y in xrange(data_time) ] for x in data_list ]
	Validset =  [[ rawData[x][y] for y in xrange(data_time) ] for x in valid_list ]
	return Dataset,Validset

Dataset,Validset = slicing_valid(csvdata)
Dataset_sliced = np.array(Dataset)
Validset_sliced = np.array(Validset)
# transform dataset shape(transpose)
def transform(Set,data_num):
	for x in xrange(data_num):
		if x==0:
			_set = np.array(Set[0:data_feature])
		else:
			_set = np.concatenate((_set,np.array(Set[x*data_feature:(x+1)*data_feature])),axis=1)
	return _set

Dataset = transform(csvdata,total_si*12)
Validset = transform(Validset_sliced,valid_si*12)

# data normalization
Data_norm = np.copy(Dataset)
Valid_norm = np.copy(Validset)
data_number = csvdata.shape[0]/data_time # total data number

def data_normalization(dat):
	time_total = dat.shape[1]
	data_feature = dat.shape[0]
	normalize = np.zeros((data_feature,2)) # mean and variation
	for x in xrange(data_feature):
		for y in xrange(time_total):
			normalize[x][0] = normalize[x][0] + dat[x][y]/time_total
			normalize[x][1] = normalize[x][1] + dat[x][y]*dat[x][y]/time_total

	for x in xrange(data_feature):
		variation = abs(normalize[x][1]-normalize[x][0]*normalize[x][0])
		normalize[x][1] = math.sqrt(variation)
	for x in xrange(data_feature):
		for y in xrange(time_total):
			if(normalize[x][1] > 0):
				dat[x][y] = (dat[x][y] - normalize[x][0])/normalize[x][1]
	return normalize,dat
	
data_normalize,Data_norm = data_normalization(Data_norm)
_,Valid_norm = data_normalization(Valid_norm)
PM_mean = data_normalize[9][0]
PM_var = data_normalize[9][1]

##########################################################################
# weight and bias initialization
path = "./"
npy_files = [f for f in os.listdir(path) if f.endswith(".npy")]
smooth = 0.00000005
if ("weight_mf.npy" in npy_files) and ("bias_mf.npy" in npy_files):
	weight = np.load("weight_mf.npy")
	bias = np.load("bias_mf.npy")
else:
	weight = np.array([ [random.gauss(0, 0.1)*pow(1.1,x) for x in xrange(time)] for y in xrange(data_feature)])
	for x in xrange(time):
		weight[9][x] = random.gauss(0, 0.5)*pow(1.1,x) 
	bias = random.gauss(0, 0.2)
# validation test --> generating batch
def generating_valid():
	batch_number = 20
	rand_ = np.zeros((batch_number),dtype=int)
	loss = 0.0
	for z in xrange(batch_number):
		i = random.randint(0,11)
		rand_[z] = random.randint(i*valid_si*data_time,(i+1)*valid_si*data_time-time-1)
		if(z==0):
			batch = [[[ Validset[y][rand_[z]+x] for x in xrange(time)] for y in xrange(data_feature) ]]
		else:
			batch.append([[ Validset[y][rand_[z]+x] for x in xrange(time)] for y in xrange(data_feature)])
	batch = np.array(batch)
	
	for z in xrange(batch_number):
		label = Validset[9][rand_[z] + time]
		loss = loss + pow( (np.multiply(weight,batch[z]).sum() + bias - label),2 )
	loss = loss/batch_number
	return loss

# set training condition
learning_rate = 0.00000008
reg = 0.0
step = 200
batch_number = 40
epoch = 3000
test_epoch = 10
# previous trained information loaded 
if ("info_mf.npy" in npy_files):
	info = np.load("info_mf.npy")
else: 
	info = np.zeros((1,2))
# training
# Dataset = Data_norm
for j in xrange(epoch): 
	current_info = np.zeros((1,2)) 
	train_Loss = 0.0
	for i in xrange(step):
		# generating batch
		rand_ = np.zeros((batch_number),dtype=int) 

		# pick up one random chosen time and fetch up the successive data
		for z in xrange(batch_number):
			l = random.randint(0,11)
			rand_[z] = random.randint(l*data_si*data_time,(l+1)*data_si*data_time-time)
			if z==0:
				batch = [[[ Dataset[y][rand_[z]+x] for x in xrange(time)] for y in xrange(data_feature) ]]
			else:
				batch.append([[ Dataset[y][rand_[z]+x] for x in xrange(time)] for y in xrange(data_feature)])
	
		batch = np.array(batch)
		# gradient descent
		grad_w = np.zeros((data_feature,time))
		grad_b = 0.0
		for z in xrange(batch_number):
			label = Dataset[9][rand_[z]+time]
			temp = 2*(np.multiply(weight,batch[z]).sum() + bias - label)
			grad_b = grad_b + temp/batch_number
			for x in xrange(data_feature):
				for y in xrange(time):
					current_grad = temp*batch[z][x][y]/batch_number 
					grad_w[x][y] = grad_w[x][y] + current_grad + 2*reg*weight[x][y]
	
		# update new bias
		bias = bias - learning_rate*(grad_b + 2*reg*bias)
		
		# updatw new weight
		for x in xrange(data_feature):
			for y in xrange(time):
				weight[x][y] = weight[x][y] - learning_rate*grad_w[x][y] 
		# training data test
		loss = 0.0
		for z in xrange(batch_number):
			label = Dataset[9][rand_[z]+time]
			loss = loss + pow((np.multiply(weight,batch[z]).sum() + bias - label),2)
		loss = loss/batch_number
		train_Loss = train_Loss + loss
	
	train_Loss = train_Loss / step
	print "Loss = ",train_Loss	
	
	test_Loss = 0.0
	for k in xrange(test_epoch):
		test_Loss = test_Loss + generating_valid()
	test_Loss = test_Loss/test_epoch
	print "ValidationLoss = ",test_Loss
	current_info[0][1] = test_Loss
	current_info[0][0] = info[info.shape[0]-1][0] + 1
	info = np.concatenate((info,current_info),axis=0)
	np.save("weight_mf.npy", weight)
	np.save("bias_mf.npy", bias)
	np.save("info_mf.npy", info)
	if (test_Loss < 2):
		print ("Done!!!!!!!!!")

# save weight and bias in npy file
np.save("weight_mf.npy", weight)
np.save("bias_mf.npy", bias)
###############################################
# read in testset from csvfile
flag = 2
filename = 'test_X.csv'
csvtest = csv_read(2,flag,filename)
test_number = csvtest.shape[0]/data_feature

# generating prediction
filename = 'kaggle_best.csv'
with open(filename, 'wb') as csvfile:
	spamwriter = csv.writer(csvfile)
	spamwriter.writerow(["id"] + ["value"])
	for x in xrange(test_number):
		test_data = csvtest[x*data_feature:(x+1)*data_feature][:]
		########## To change
		predict = (np.multiply(weight,test_data).sum() + bias) 
		##########
		spamwriter.writerow(["id_%d"%(x)] + ["%f"%(predict)])	
