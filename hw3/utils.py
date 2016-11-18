try: import cPickle as pickle
except: import pickle
import numpy as np
import os
from keras import backend as K
from keras.models import model_from_json
from keras.utils import np_utils
#########################################################
# Remember to set the backend to theano
# KERAS_BACKEND=theano python -c "from keras import backend"

def loading_Data(path,spiltNum=400):
	# input image dimensions
	img_rows, img_cols, img_chns = 32, 32, 3
	###################Loading data##########################
	print "Loading data from pickle file"
	labeledData = pickle.load(open(path+"all_label.p",'rb'))
	unlabeledData = pickle.load(open(path+"all_unlabel.p",'rb'))
	testData = pickle.load(open(path+"test.p",'rb'))
	testData = testData['data']
	testData = np.array(testData)
	###############Information about dataset#################
	classNum = len(labeledData)
	dataNumPerClass = len(labeledData[0])
	totalNum = classNum*dataNumPerClass
	totalUnlabeledNum = len(unlabeledData)
	totalTestNum = testData.shape[0]
	##################Reshaping dataset######################
	print "Reshaping data to correct format"
	trainData = np.reshape(np.array(labeledData),(totalNum,img_chns,img_rows,img_cols))
	trainUnlabeledData = np.reshape(np.array(unlabeledData),(totalUnlabeledNum,img_chns,img_rows,img_cols))
	testData = np.reshape(testData,(totalTestNum,channelNum,img_hei,img_wid))
	trainUnlabeledData = np.append(trainUnlabeledData,testData,axis=0)
	print "testData is used as unlabeledData!"
	trainLabel = [ i/dataNumPerClass for i in xrange(totalNum) ]
	if K.image_dim_ordering() == 'th':
		print "Backend: theano"
	else:
    	print "Backend: tensorflow"
		trainData = np.moveaxis(np.array(trainData), 1, -1)
		trainUnlabeledData = np.moveaxis(np.array(trainUnlabeledData), 1, -1)

	assert len(trainData) == len(trainLabel) and len(trainData) == totalNum
	#########################################################
	X_train = trainData.astype('float32')
	unlabeledX_train = trainUnlabeledData.astype('float32')
	X_train /= 255
	unlabeledX_train /= 255
	# convert class vectors to binary class matrices
	#########################################################
	Y_train = np_utils.to_categorical(np.array(trainLabel), classNum)

	p = np.random.permutation(totalNum)
	X_train = X_train[p]
	Y_train = Y_train[p]
	X_valid = X_train[:spiltNum]
	Y_valid = Y_train[:spiltNum]
	X_train = X_train[spiltNum:]
	Y_train = Y_train[spiltNum:]
	############Print out dataset information################
	print 'X_train shape:', X_train.shape
	print X_train.shape[0], 'train samples'
	#######################cifar10###########################
	print 'X_valid shape:', X_valid.shape
	print X_valid.shape[0], 'validation samples'
	#########################################################
	return (X_train,Y_train), unlabeledX_train, (X_valid,Y_valid)

def load_Model(filename='autoencoder_model.json'):
	################Construct the model######################
	root = "./"
	current_files = [f for f in os.listdir(root) if (f.endswith(".h5") or f.endswith(".json"))]
	if ('.json' not in filename):
		filename = filename + '.json'
	if (filename not in current_files):
		filename = 'autoencoder_model.json'
	if (filename not in current_files):
		return False
	# load json and create model
	json_file = open(filename, 'r')
	loaded_model_json = json_file.read()
	json_file.close()
	model = model_from_json(loaded_model_json)
	return model