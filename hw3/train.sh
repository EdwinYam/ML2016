KERAS_BACKEND=theano THEANO_FLAGS=mode=FAST_RUN,device=gpu0,floatX=float32 python selfAutoEncoder.py $1 $2 -c "from keras import backend"
