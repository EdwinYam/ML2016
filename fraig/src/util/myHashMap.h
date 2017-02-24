/****************************************************************************
  FileName     [ myHashMap.h ]
  PackageName  [ util ]
  Synopsis     [ Define HashMap and Cache ADT ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2009-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef MY_HASH_MAP_H
#define MY_HASH_MAP_H

#include <vector>

using namespace std;

// TODO: (Optionally) Implement your own HashMap and Cache classes.

//-----------------------
// Define HashMap classes
//-----------------------
// To use HashMap ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.

class HashKey
{
public:
	HashKey(unsigned a, unsigned b) : _a(a),_b(b) { assert(a <= b); }

	size_t operator() () const { return (size_t(_a) << 32) + size_t(_b); }
	bool operator == (const HashKey& k) const { return (_a == k._a && _b == k._b); }
	bool invert(const HashKey& k) { return _a!=k._a && _b!=k._b; }
private:
	unsigned _a,_b;
};
/*****************************************************************/
template <class HashKey, class HashData>
class HashMap
{
typedef pair<HashKey, HashData> HashNode;

public:
	HashMap(size_t b=0) : _numBuckets(0), _buckets(0) { if (b != 0) init(b); }
	~HashMap() { reset(); }

	// [Optional] TODO: implement the HashMap<HashKey, HashData>::iterator
	// o An iterator should be able to go through all the valid HashNodes
	//   in the HashMap
	// o Functions to be implemented:
	//   - constructor(s), destructor
	//   - operator '*': return the HashNode
	//   - ++/--iterator, iterator++/--
	//   - operators '=', '==', !="
	//
	class iterator
	{
		friend class HashMap<HashKey, HashData>;

	public:
		iterator(vector<HashNode>* n = 0, size_t index = 0, size_t num = 0)
		:_bucket(n),_bucketIndex(index),_colIndex(0),_numBucket(num){
			for (; _bucketIndex<_numBucket; ++_bucketIndex) {
				if (_bucket[_bucketIndex].size()!=0) break;
			}
		}
		iterator(const iterator& i)
		:_bucket(i._bucket),_bucketIndex(i._bucketIndex),_colIndex(i._colIndex),_numBucket(i._numBucket) {}
		~iterator() {}

		const HashNode& operator * () const { assert(_bucketIndex!=_numBucket); return _bucket[_bucketIndex][_colIndex]; }
		HashNode& operator * () { assert(_bucketIndex!=_numBucket); return _bucket[_bucketIndex][_colIndex]; }
		iterator& operator ++ () { 
			if (_bucketIndex == _numBucket) return *(this);
			if (++_colIndex < _bucket[_bucketIndex].size()) { return *(this); }
			_colIndex = 0;
			++_bucketIndex;
			for (; _bucketIndex<_numBucket; ++_bucketIndex) {
				if (_bucket[_bucketIndex].size()!=0) return *(this);
			}
			assert(_bucketIndex == _numBucket);
			return *(this); 
		}
		iterator operator ++ (int) { iterator _temp = *(this); ++(*this); return _temp; }
		iterator& operator -- () { 
			if (--_colIndex >= 0) { return *(this); }
			size_t tempIndex = _bucketIndex - 1;
			for (; tempIndex>=0; --tempIndex) {
				if (_bucket[tempIndex].size()!=0) { 
					_bucketIndex = tempIndex; 
					_colIndex =  _bucket[_bucketIndex].size() - 1;
					return *(this); 
				}
			}
			assert(tempIndex == -1);
			return *(this); 
		}
		iterator operator -- (int) { iterator _temp = *(this); --(*this); return _temp; }

		iterator& operator = (const iterator& i) { 
			_bucket = i._bucket;
			_bucketIndex = i._bucketIndex; 
			_colIndex = i._colIndex;
			_numBucket = i._numBucket;
			return *(this); 
		}

		bool operator != (const iterator& i) const {  
			return !(*(this)==i);
		}
		bool operator == (const iterator& i) const {
			return (_bucketIndex==i._bucketIndex && _colIndex==i._colIndex && _numBucket==i._numBucket); 
		}
	private:
		vector<HashNode>* _bucket;
		size_t _bucketIndex, _colIndex, _numBucket;
	};
	//

	void init(size_t b) { reset(); _numBuckets = b; _buckets = new vector<HashNode>[b]; }
	void reset() {
		_numBuckets = 0;
		if (_buckets) { delete [] _buckets; _buckets = 0; }
	}
	void clear() {
		for (size_t i = 0; i < _numBuckets; ++i) _buckets[i].clear();
	}
	size_t numBuckets() const { return _numBuckets; }

	vector<HashNode>& operator [] (size_t i) { return _buckets[i]; }
	const vector<HashNode>& operator [](size_t i) const { return _buckets[i]; }

	// TODO: implement these functions
	//
	// Point to the first valid data
	iterator begin() const { return iterator(_buckets,0,_numBuckets); }
	// Pass the end
	iterator end() const { return iterator(_buckets,_numBuckets,_numBuckets); }
	// return true if no valid data
	bool empty() const { return (begin()==end()); }
	// number of valid data
	size_t size() const { size_t s = 0; for (iterator it = begin(); it!=end(); ++it) ++s; return s; }

	// check if k is in the hash...
	// if yes, return true;
	// else return false;
	bool check(const HashKey& k, bool inv = false) const { 
		size_t bucketIndex = bucketNum(k);
		for (size_t i = 0; i<_buckets[bucketIndex].size(); ++i) {
			if (_buckets[bucketIndex][i].first == k) {
				if (inv) return k.invert(_buckets[bucketIndex][i].first);
				return true;
			}
		}
		return false;
	}

	// query if k is in the hash...
	// if yes, replace d with the data in the hash and return true;
	// else return false;
	bool query(const HashKey& k, HashData& d) const {
		size_t bucketIndex = bucketNum(k);
      for (size_t i = 0; i<_buckets[bucketIndex].size(); ++i) {
         if (_buckets[bucketIndex][i].first == k) { d = _buckets[bucketIndex][i].second; return true; }
      }
      return false;
	}

	// update the entry in hash that is equal to k (i.e. == return true)
	// if found, update that entry with d and return true;
	// else insert d into hash as a new entry and return false;
	bool update(const HashKey& k, HashData& d) {
		size_t bucketIndex = bucketNum(k);
      for (size_t i = 0; i<_buckets[bucketIndex].size(); ++i) {
         if (_buckets[bucketIndex][i].first == k) { _buckets[bucketIndex][i].second = d; return true; }
      }
      _buckets[bucketIndex].push_back(HashNode(k,d)); 
      return false;
	}

	// return true if inserted d successfully (i.e. k is not in the hash)
	// return false is k is already in the hash ==> will not insert
	bool insert(const HashKey& k, const HashData& d) {
		size_t bucketIndex = bucketNum(k);
      for (size_t i = 0; i<_buckets[bucketIndex].size(); ++i) {
         if (_buckets[bucketIndex][i].first == k) return false;
      }
      _buckets[bucketIndex].push_back(HashNode(k,d));
      return true;
	}

	// return true if removed successfully (i.e. k is in the hash)
	// return fasle otherwise (i.e. nothing is removed)
	bool remove(const HashKey& k) {
		size_t bucketIndex = bucketNum(k);
      for (size_t i = 0; i<_buckets[bucketIndex].size(); ++i) {
         if (_buckets[bucketIndex][i].first == k) {
            _buckets[bucketIndex].erase(_buckets[bucketIndex].begin() + i);
            return true;
         }
      }
      return false; 
	}

private:
	// Do not add any extra data member
	size_t                   _numBuckets;
	vector<HashNode>*        _buckets;

	size_t bucketNum(const HashKey& k) const {
		return (k() % _numBuckets); }

};


//---------------------
// Define Cache classes
//---------------------
// To use Cache ADT, you should define your own HashKey class.
// It should at least overload the "()" and "==" operators.
//
// class CacheKey
// {
// public:
//    CacheKey() {}
//    
//    size_t operator() () const { return 0; }
//   
//    bool operator == (const CacheKey&) const { return true; }
//       
// private:
// }; 
// 
template <class CacheKey, class CacheData>
class Cache
{
typedef pair<CacheKey, CacheData> CacheNode;

public:
	Cache() : _size(0), _cache(0) {}
	Cache(size_t s) : _size(0), _cache(0) { init(s); }
	~Cache() { reset(); }

	// NO NEED to implement Cache::iterator class

	// TODO: implement these functions
	//
	// Initialize _cache with size s
	void init(size_t s) { reset(); _size = s; _cache = new CacheNode[s]; }
	void reset() {  _size = 0; if (_cache) { delete [] _cache; _cache = 0; } }

	size_t size() const { return _size; }

	CacheNode& operator [] (size_t i) { return _cache[i]; }
	const CacheNode& operator [](size_t i) const { return _cache[i]; }

	// return false if cache miss
	bool read(const CacheKey& k, CacheData& d) const {
		size_t i = k() % _size;
		if (k == _cache[i].first) {
			d = _cache[i].second;
			return true;
		}
		return false;
	}
	// If k is already in the Cache, overwrite the CacheData
	void write(const CacheKey& k, const CacheData& d) {
		size_t i = k() % _size;
		_cache[i].first = k;
		_cache[i].second = d;
	}

private:
	// Do not add any extra data member
	size_t         _size;
	CacheNode*     _cache;
};

#endif // MY_HASH_H
