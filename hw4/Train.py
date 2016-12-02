import re
import codecs
import nltk
from sys import argv
from numpy import zeros, array
from collections import Counter
from sklearn.cluster import KMeans , MiniBatchKMeans
from sklearn import preprocessing
from sklearn.decomposition import PCA
from gensim.models import Word2Vec

dataPath = argv[1]
predictFileName = argv[2]
if 'csv' not in predictFileName:
	predictFileName = predictFileName + '.csv'

#### Loading Data ####
print 'Loading data from disk.'
title_TXT = ''
docs_TXT = ''
total_TXT = ''

with open(dataPath + 'title_StackOverflow.txt', 'r') as f:
	title_TXT = f.read()
f.close()
with open(dataPath + 'docs.txt', 'r') as f:
	docs_TXT = f.read()
f.close()
with open(dataPath + 'stopWords.txt', 'r') as f:
	stopWords = f.read()
f.close()

print 'Preprocessing data.'
title_TXT = title_TXT.lower()
docs_TXT = docs_TXT.lower()
stopWords = stopWords.lower()
total_TXT = title_TXT + docs_TXT

import string
replace_punctuation = string.maketrans(string.punctuation, ' '*len(string.punctuation))
total_TXT = total_TXT.translate(replace_punctuation)
tokens = nltk.word_tokenize(total_TXT)
stopWords = nltk.word_tokenize(stopWords)

print 'Stemming and Lemmatization.' 
from nltk.stem.porter import PorterStemmer
from nltk.stem import WordNetLemmatizer
porter_stemmer = PorterStemmer()
wordnet_lemmatizer = WordNetLemmatizer()
for i in xrange(len(tokens)):
	tokens[i] = porter_stemmer.stem(tokens[i].decode('utf-8'))
	tokens[i] = wordnet_lemmatizer.lemmatize(tokens[i])
for i in xrange(len(stopWords)):
	stopWords[i] = porter_stemmer.stem(stopWords[i])
	stopWords[i] = wordnet_lemmatizer.lemmatize(stopWords[i])
count_NumPerToken = Counter(tokens)
print 'There are ', sum(count_NumPerToken.values()), 'tokens.'
print 'There are ', len(count_NumPerToken), 'kinds of tokens.'

print 'Making list of stopwords.'
themeWords = ['wordpress', 'oracle', 'svn', 'apache', 'excel', 'matlab', 'visual', 'studio',
'cocoa', 'osx', 'bash', 'spring', 'hibernate', 'scala', 'sharepoint', 'ajax', 'qt', 'drupal',
'linq', 'haskell', 'magento']
for i in xrange(len(themeWords)):
	themeWords[i] = porter_stemmer.stem(themeWords[i])
	themeWords[i] = wordnet_lemmatizer.lemmatize(themeWords[i])

stop_Num = 200
useless_Word = count_NumPerToken.most_common(stop_Num)
for i in xrange(stop_Num):
	if (useless_Word[i][0] not in stopWords and useless_Word[i][0] not in themeWords):
		stopWords.append(useless_Word[i][0])

print 'Filtering by stopWord list.'
sentences = []
f = codecs.open(dataPath + 'title_StackOverflow.txt' , 'r' , 'utf-8')
for lines in f:
	tmp = re.findall('\w+', lines)
	temp = []
	for word in tmp:
		_word = word.lower()
		_word = porter_stemmer.stem(_word)
		_word = wordnet_lemmatizer.lemmatize(_word)
		if _word not in stopWords:
			temp.append(_word)
	if len(temp) > 0:
		sentences.append(temp)
f.close()

f = codecs.open(dataPath + 'docs.txt' , 'r' , 'utf-8')
for lines in f:
	tmp = re.findall('\w+', lines)
	temp = []
	for word in tmp:
		_word = word.lower()
		_word = porter_stemmer.stem(_word)
		_word = wordnet_lemmatizer.lemmatize(_word)
		if _word not in stopWords:
			temp.append(_word)
	if len(temp) > 0:
		sentences.append(temp)
f.close()

print 'Training word2vec...'
model = Word2Vec(sentences , min_count = 5 , size = 400 , iter = 40)

vec = zeros(shape = (20000 , 400))
f = codecs.open(dataPath + 'title_StackOverflow.txt' , 'r' , 'utf-8')
i = 0
for lines in f:
	tmp = re.findall('\w+' , lines)
	temp = []
	for word in tmp:
		_word = word.lower()
		_word = porter_stemmer.stem(_word)
		_word = wordnet_lemmatizer.lemmatize(_word)
		if _word in model.vocab:
			temp.append(_word)
	count = 0
	for word in temp:
		count = count + 1
		vec[i] = vec[i] + model[word.decode('utf-8')]
	if count > 0:
		vec[i] = vec[i]/count
	else:
		vec[i] = vec[i - 1]
	i = i + 1
f.close()

pca = PCA(n_components = 30)
_vec = pca.fit_transform(vec)
VEC = preprocessing.normalize(_vec , norm='l2')

testID = []
with open(dataPath + 'check_index.csv', 'r') as f:
    i = 0
    for lines in f:
        if i > 0:
            lines = lines.split(',')
            tmp = []
            tmp.append(int(lines[0]))
            tmp.append(int(lines[1]))
            tmp.append(int(lines[2]))
            testID.append(tmp)
        i = i + 1
f.close()
testID = array(testID)

kmeans = MiniBatchKMeans(init='k-means++', n_clusters=29, n_init=40 , batch_size=1000).fit(VEC)
ans = kmeans.fit_predict(VEC)

print 'Predicting...'
totalSize = 5000000
with open(predictFileName, 'w') as f:
    f.write('ID,Ans')
    f.write('\n')
    for i in xrange(totalSize):
    	f.write(str(i))
        f.write(',')
    	if ans[testID[i][1]] == ans[testID[i][2]]:
    		f.write(str(1))
    	else:
    		f.write(str(0))
        f.write('\n')
f.close()