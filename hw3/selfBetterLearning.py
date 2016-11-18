import os
import csv 
import numpy as np
from sys import argv
import matplotlib.pyplot as plt
import matplotlib.image as ig
from keras.preprocessing.image import ImageDataGenerator
from keras.models import Sequential
from keras.layers import Convolution2D, MaxPooling2D, Dense, Activation, Dropout, Flatten
from keras.optimizers import SGD
from keras.utils import np_utils
from keras.models import model_from_json
from lsuv_init import LSUVinit
from utils import loading_Data

dataFilePath = argv[1]
###################Loading data##########################
(X_train,Y_train), unlabeledX_train, (X_valid,Y_valid) = loading_Data(dataFilePath)
###############Information about dataset#################
classNum = 10
totalNum = len(X_train)
totalUnlabeledNum = len(unlabeledX_train)
channelNum = 3
img_wid = 32
img_hei = 32

# Below code is used to check each picture in the dataset
'''
def show_picture(number):
  image = np.moveaxis(np.array(trainData[number],dtype=np.float32), 0, -1)
  image = np.roll(image/255, 2, axis=-1)
  print image.shape
  plt.imshow(image)
  plt.show()
for i in xrange(totalNum):
  if (i%40==0):
    show_picture(i)
'''
################Construct the model######################
modelname = argv[2]
json_filename = modelname + ".json"
h5_filename = modelname + ".h5"
root = "./"
current_files = [f for f in os.listdir(root) if (f.endswith(".h5") or f.endswith(".json"))]
if ('deepSelf_model.json' in current_files):
  # load json and create model
  json_file = open('deepSelf_model.json', 'r')
  loaded_model_json = json_file.read()
  json_file.close()
  model = model_from_json(loaded_model_json)

else:
  model = Sequential()
  model.add(Convolution2D(64, 3, 3, border_mode='same',input_shape=X_train.shape[1:]))
  model.add(Activation('relu'))
  model.add(Convolution2D(64, 3, 3, border_mode='same'))
  model.add(Activation('relu'))
  model.add(MaxPooling2D(pool_size=(2, 2)))
  model.add(Dropout(0.25))

  model.add(Convolution2D(128, 3, 3, border_mode='same'))
  model.add(Activation('relu'))
  model.add(Convolution2D(128, 3, 3, border_mode='same'))
  model.add(Activation('relu'))
  model.add(MaxPooling2D(pool_size=(2, 2)))
  model.add(Dropout(0.25))

  model.add(Convolution2D(256, 3, 3, border_mode='same'))
  model.add(Activation('relu'))
  model.add(Convolution2D(256, 3, 3, border_mode='same'))
  model.add(Activation('relu'))
  model.add(Convolution2D(256, 3, 3, border_mode='same'))
  model.add(Activation('relu'))
  model.add(Convolution2D(256, 3, 3, border_mode='same'))
  model.add(Activation('relu'))
  model.add(MaxPooling2D(pool_size=(2, 2)))
  model.add(Dropout(0.25))

  model.add(Flatten())
  model.add(Dense(1024))
  model.add(Activation('relu'))
  model.add(Dropout(0.5))
  model.add(Dense(1024))
  model.add(Activation('relu'))
  model.add(Dropout(0.5))
  model.add(Dense(classNum))
  model.add(Activation('softmax'))  
###################Hyper parameters######################
data_augmentation = True
batch_size = 32
nb_epoch = 40  
#############Initialization for weights##################
if ('deepSelf_model.h5' in current_files):  
  # load weights into new model
  model.load_weights("deepSelf_model.h5")
  print("Loaded model from disk")
else:
  model = LSUVinit(model,X_train[:batch_size,:,:,:])

# Train the model using SGD + momentum (how original).
sgd = SGD(lr=0.01, decay=1e-6, momentum=0.9, nesterov=True)
model.compile(loss='categorical_crossentropy', optimizer=sgd, metrics=['accuracy'])
######################Training###########################
def labeled_Training(nb_epoch,batch_size,X_train,Y_train):
  if not data_augmentation:
    print('Not using data augmentation.')
    history = model.fit(X_train, Y_train,
              batch_size=batch_size,
              nb_epoch=nb_epoch,
              validation_data=(X_valid,Y_valid),
              shuffle=True)
  else:
    print('Using real-time data augmentation.')

    # this will do preprocessing and realtime data augmentation
    datagen = ImageDataGenerator(
        featurewise_center=False,  # set input mean to 0 over the dataset
        samplewise_center=False,  # set each sample mean to 0
        featurewise_std_normalization=False,  # divide inputs by std of the dataset
        samplewise_std_normalization=False,  # divide each input by its std
        zca_whitening=False,  # apply ZCA whitening
        rotation_range=5,  # randomly rotate images in the range (degrees, 0 to 180)
        width_shift_range=0.1,  # randomly shift images horizontally (fraction of total width)
        height_shift_range=0.1,  # randomly shift images vertically (fraction of total height)
        horizontal_flip=True,  # randomly flip images
        vertical_flip=False,
        fill_mode='nearest')  # randomly flip images

    # compute quantities required for featurewise normalization
    # (std, mean, and principal components if ZCA whitening is applied)
    datagen.fit(X_train)

    # fit the model on the batches generated by datagen.flow()
    history = model.fit_generator(datagen.flow(X_train, Y_train,
                        batch_size=batch_size),
                        samples_per_epoch=X_train.shape[0],
                        nb_epoch=nb_epoch,
                        validation_data=(X_valid,Y_valid))
  return history
#################Pretrain################################
_History = labeled_Training(60,batch_size,X_train,Y_train)
#################Adding unlabeles data###################
i = 0
max_iter = 5
prob_threshold = 0.80
proba = model.predict_proba(unlabeledX_train, batch_size=32)
unlabeledY = np.argmax(proba, axis=1)
unlabeledY_old = np.array([])


#re-train, labeling unlabeled instances with model predictions, until convergence
while (len(unlabeledY_old) == 0 or np.any(unlabeledY!=unlabeledY_old)) and i < max_iter:
  unlabeledY_old = np.copy(unlabeledY)
  tempY = np_utils.to_categorical(unlabeledY_old, classNum)
  uidx = np.where(proba > prob_threshold)[0]       
  tempX_train = np.vstack((X_train, unlabeledX_train[uidx, :]))
  tempY_train = np.vstack((Y_train, tempY[uidx, :]))
  _history = labeled_Training(nb_epoch,batch_size,tempX_train,tempY_train)
  _History.history['acc'] = np.append(_History.history['acc'],_history.history['acc'],axis=0)
  _History.history['val_acc'] = np.append(_History.history['val_acc'],_history.history['val_acc'],axis=0)
  _History.history['loss'] = np.append(_History.history['loss'],_history.history['loss'],axis=0)
  _History.history['val_loss'] = np.append(_History.history['val_loss'],_history.history['val_loss'],axis=0)

  proba = model.predict_proba(unlabeledX_train, batch_size =32)
  unlabeledY = np.argmax(proba, axis=1)
  i += 1

####################History##############################
# list all data in history
print(_History.history.keys())
# summarize history for accuracy
plt.plot(_History.history['acc'])
plt.plot(_History.history['val_acc'])
plt.title('model accuracy')
plt.ylabel('accuracy')
plt.xlabel('epoch')
plt.legend(['train', 'test'], loc='upper left')
plt.savefig('deepSelfAcc.png')
plt.show()

# summarize history for loss
plt.plot(_History.history['loss'])
plt.plot(_History.history['val_loss'])
plt.title('model loss')
plt.ylabel('loss')
plt.xlabel('epoch')
plt.legend(['train', 'test'], loc='upper left')
plt.savefig('deepSelfLoss.png')
plt.show()
#################Serialize model to JSON#################
model_json = model.to_json()
with open(json_filename, "w") as json_file:
    json_file.write(model_json)
################Serialize weights to HDF5################
model.save_weights(h5_filename)
print("Saved model to disk")