try: import cPickle as pickle
except: import pickle
import numpy as np 
import csv
import os
from sys import argv
from keras import backend as K
from keras.models import model_from_json
path = argv[1]
modelname = argv[2]
predictfile = argv[3]
if '.csv' not in predictfile:
	predictfile += '.csv'

classNum = 10
channelNum = 3
img_wid = 32
img_hei = 32
###############generating prediction###############
testData = pickle.load(open(path+"test.p",'rb'))
testData = testData['data']
testData = np.array(testData)
totalTestNum = testData.shape[0]
testData = np.reshape(testData,(totalTestNum,channelNum,img_hei,img_wid))
if K.image_dim_ordering() == 'th':
	print "Backend: theano"
else:
	print "Backend: tensorflow"
	testData = np.moveaxis(np.array(testData), 1, -1)
X_test = testData.astype('float32')
X_test /= 255
#################Loading model#####################
json_file = open(modelname + '.json', 'r')
loaded_model_json = json_file.read()
json_file.close()
model = model_from_json(loaded_model_json)
model.load_weights(modelname + '.h5')
print("Loaded model from disk")
###################################################
classes = model.predict_classes(X_test, batch_size=32)

with open(predictfile, 'w+') as csvfile:
	spamwriter = csv.writer(csvfile)
	spamwriter.writerow(["ID"] + ["class"])
	for x in xrange(totalTestNum):
		########## To change
		predict = classes[x]
		##########
		spamwriter.writerow(["%d"%(x)] + ["%d"%(predict)])
