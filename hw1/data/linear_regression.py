import csv
import numpy as np
import math
import random
from tempfile import TemporaryFile
from datetime import datetime
random.seed(datetime.now())

# csv data read in
filename = 'train.csv'
flag = 1
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
csvdata = csv_read(3,flag,filename)

# seprating data set and validation set
data_time = 24
data_feature = 18
data_si = 15	
valid_si = 5
total_si = data_si + valid_si
data_list = []
valid_list = []
for x in xrange(csvdata.shape[0]):
	if (x/data_feature%total_si >= data_si) and (x/data_feature%total_si < total_si):
		valid_list.append(x)
	else:	
		data_list.append(x)
Dataset = [[ csvdata[x][y] for y in xrange(data_time) ] for x in data_list ]
Validset =  [[ csvdata[x][y] for y in xrange(data_time) ] for y in valid_list ]
Dataset = np.array(Dataset)
Validset = np.array(Dataset)
# data normalization
data = np.copy(Dataset)
data_number = data.shape[0]/data_feature
data_normalize = np.zeros((data_feature,2)) # mean and variation
for x in xrange(data_number):
	for y in xrange(data_feature):
		for z in xrange(data_time):
			data_normalize[y][0] = data_normalize[y][0] + data[x*data_feature + y][z]/(data_number*data_time)
			data_normalize[y][1] = data_normalize[y][1] + data[x*data_feature + y][z]*data[x*data_feature + y][z]/(data_number*data_time)

for x in xrange(data_feature):
	variation = abs(data_normalize[x][1]-data_normalize[x][0]*data_normalize[x][0])
	data_normalize[x][1] = math.sqrt(variation)
for x in xrange(data_number):
        for y in xrange(data_feature):
                for z in xrange(data_time):
                        data[x*data_feature + y][z] = (data[x*data_feature + y][z] - data_normalize[y][0])/data_normalize[y][1]

PM_mean = data_normalize[9][0]
PM_var = data_normalize[9][1]
time = 9
# weight and bias initialization
weight = np.array([ [random.gauss(0, 0.1) for x in xrange(time)] for y in xrange(data_feature)])
weight_adam = np.zeros((data_feature,time))
bias = random.gauss(0, 0.1)
bias_adam = 0
# set training condition
learning_rate = 0.1
step = 1000
batch_number = 40

# transform dataset shape
def transform(Set,data_num,data_t,data_f):
	for x in xrange(data_num):
		if x==0:
			_set = Set[0:data_f]
		else:
			_set = np.concatenate((_set,Set[x*data_f:(x+1)*data_f]),axis=1)
	return _set
Dataset = transform(Dataset,180,data_time,data_feature)
Validset = transform(Validset,60,data_time,data_feature)
# train
for i in xrange(step):
	# generating batch
	rand_ = np.zeros((batch_number),dtype=int)
	for z in xrange(batch_number):
		l = random.randint(0,11)
		rand_[z] = random.randint(l*data_si*data_time,(l+1)*data_si*data_time-1-time)
		if z==0:
			batch = [[[ Dataset[y][rand_[z]+x] for x in xrange(time)] for y in xrange(data_feature) ]]
		else:
			batch.append([[ Dataset[y][rand_[z]+x] for x in xrange(time)] for y in xrange(data_feature)])
	
	batch = np.array(batch)
	# gradient descent
	grad_w = np.zeros((data_feature,time))	
	grad_b = 0
	for z in xrange(batch_number):
		label = Dataset[9][rand_[z]+1]
		for x in xrange(data_feature):
			for y in xrange(time):
				temp = 2*(np.multiply(weight,batch[z]).sum()+bias-label)
				grab_b = grad_b + temp/batch_number 
				grad_w[x][y] = grad_w[x][y] + temp*batch[z][x][y]/batch_number + 2*weight[x][y]
	bias_adam = math.sqrt(pow(bias_adam,2) + pow(grad_b,2))
	bias = bias - learning_rate*grab_b
	for x in xrange(data_feature):
		for y in xrange(time):
			weight_adam[x][y] = math.sqrt(pow(weight_adam[x][y],2) + pow(grad_w[x][y],2))
			weight[x][y] = weight[x][y] - learning_rate*grad_w[x][y]/weight_adam[x][y]
	# test 
	loss = 0
	for z in xrange(batch_number):
		label = Dataset[9][rand_[z]+1];
		loss = loss+pow((np.multiply(weight,batch[z]).sum() + bias - label),2)
	loss = loss/batch_number
	print ('Loss = ',loss)
	outfile = TemporaryFile()
	np.save(outfile, weight)
	np.save(outfile, bias)
	

# validation test
'''
rand_time = random.randint(0,data_time-time-1)
rand_num = random.randint(0,data_number-1)
batch = [[ csvdata[rand_num*data_feature + x][rand_time + y] for y in xrange(time)] for x in xrange(data_feature) ]
label = csvdata[rand_num*data_feature+9][rand_time+1];
loss = pow((np.multiply(weight,batch).sum() + bias - label),2)
print ('Validation test loss = ',loss)
'''
# generating batch
def generating_valid():
	batch_number = 10
	rand_ = np.zeros((batch_number),dtype=int)
	loss = 0;
	for z in xrange(batch_number):
		i = random.randint(0,11)
        	rand_[z] = random.randint(i*valid_si*data_time,(i+1)*valid_si*data_time-1-time)
        	if z==0:
	       		batch = [[[ Validset[y][rand_[z]+x] for x in xrange(time)] for y in xrange(data_feature) ]]
        	else:
        		batch.append([[ Validset[y][rand_[z]+x] for x in xrange(time)] for y in xrange(data_feature)])
        batch = np.array(batch)
	for z in xrange(batch_number):
		label = Validset[9][rand_[z]+1]
		loss = loss + pow((np.multiply(weight,batch[z]).sum() + bias - label),2)
		print ('Validation test loss = ',loss)
	loss = loss/batch_number

generating_valid()

# output test result
flag = 2
filename = 'test_X.csv'
csvtest = csv_read(2,flag,filename)
test_number = csvtest.shape[0]/data_feature

filename = 'result.csv'
with open(filename, 'wb') as csvfile:
	spamwriter = csv.writer(csvfile)
	spamwriter.writerow(["id"] + ["value"])
	for x in xrange(test_number):
		test_data = csvtest[x*data_feature:(x+1)*data_feature][:]
		predict = np.multiply(weight,test_data).sum() + bias
    		spamwriter.writerow(["id_%d"%(x)] + ["%f"%(predict)])
	
