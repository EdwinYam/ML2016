/****************************************************************************
	FileName     [ cirGate.h ]
	PackageName  [ cir ]
	Synopsis     [ Define basic gate data structures ]
	Author       [ Chung-Yang (Ric) Huang ]
	Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_GATE_H
#define CIR_GATE_H

#include <string>
#include <vector>
#include <iostream>
#include "cirDef.h"
#include "sat.h"

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

class CirGate;

//------------------------------------------------------------------------
//   Define classes
//------------------------------------------------------------------------
class CirGate
{
	friend class CirMgr;
public:
	CirGate() { _state = 0, _inDFS = 0; _pattern = 0; }
	virtual ~CirGate();

	// Basic access methods
	virtual string getTypeStr() const = 0;
	unsigned getID() const { return _id; }
	unsigned getLineNo() const { return _lineNo; }
	string getSymbolStr() const { return _symbol; }
	void setSymbolStr(const string symbol) { _symbol = symbol; }
	virtual bool isAig() const { return false; }
	virtual bool isPO() const { return false; }
	virtual bool isPI() const { return false; }
	virtual bool isCONST() const { return false; }
	virtual bool isUNDEF() const { return false; }

	// Printing functions
	virtual void printGate() const;
	void reportGate() const;
	void reportFanin(int level) const;
	void reportFanout(int level) const;
	void depthFirstSearch(int level, bool downWard, int currentlevel) const;
	
	void partialDFS(IdList& list, bool isPartial) const;
	void printDFS(unsigned& index);
	IdList getFanInID() const;
	IdList getFanOutID() const;
	void linkFanInandOut(CirGate* in, bool isInv);

	static void setGlobalRef() { ++_globalState; }
	static void setGlobalDFS() { ++_dfsState; }
	void mark2GlobalState() const { _state = _globalState; }
	void mark2GlobalState(bool inDFS) const { _state = _globalState; _inDFS = _dfsState; }
	bool isGlobalState() const { return _state == _globalState; }
	bool inDFSList() const { return _inDFS == _dfsState; }
	virtual bool simulateforGate(bool init) { return true; }

	size_t& getPattern() const { return _pattern; }

private:

protected:
	GateType _type;
	string _symbol;
	unsigned _id, _lineNo;
	GateList _fanIN, _fanOUT;
	vector<bool> _isInvIN,_isInvOUT;
	static unsigned _globalState, _dfsState;
	mutable unsigned _state, _inDFS; 
	mutable size_t _pattern;
};

class CirPI: public CirGate {
public:
	CirPI(unsigned id, unsigned lineNo) { _type = PI_GATE; _id = id; _lineNo = lineNo; _symbol = ""; }
	~CirPI() {}
	virtual string getTypeStr() const { return "PI"; }
	virtual bool isPI() const { return true; }
	void setPattern(size_t pattern) { _pattern = pattern; }
protected:
	
};

class CirPO: public CirGate {
public:
	CirPO(unsigned id, unsigned lineNo) { _type = PO_GATE; _id = id; _lineNo = lineNo; _prePattern = 0; }
	~CirPO() {}
	virtual string getTypeStr() const { return "PO"; }
	virtual bool isPO() const { return true; }
	virtual bool simulateforGate(bool init);
protected:
	size_t _prePattern;
};

class CirAIG: public CirGate {
public:
	CirAIG(unsigned id, unsigned lineNo) { _type = AIG_GATE; _id = id; _lineNo = lineNo; _prePattern = 0; }
	~CirAIG() {}
	virtual string getTypeStr() const { return "AIG"; }
	virtual bool isAig() const { return true; }
	virtual bool simulateforGate(bool init);
protected:
	size_t  _prePattern;
};

class CirCONST: public CirGate {
public:
	CirCONST() { _type = CONST_GATE; _id = 0; _lineNo = 0; _symbol = "CONST_GATE"; }
	~CirCONST() {}
	virtual string getTypeStr() const { return "CONST"; }
	virtual bool isCONST() const { return true; }
protected:
	
};

class CirUNDEF: public CirGate {
public:
	CirUNDEF(unsigned id) { _type = UNDEF_GATE; _id = id; _lineNo = 0; _pattern = 0;}
	~CirUNDEF() {}
	virtual void printGate() const {}
	virtual string getTypeStr() const { return "UNDEF"; }
	virtual bool isUNDEF() const { return true; }
protected:
	
};


class FecKey
{
public:
	FecKey(CirGate* n) : value(n->getPattern()) {}

	size_t operator() () const {
		if(~value < value) return ~value;
		return value;
 	}

	bool operator == (const FecKey& k) const { return (*this)() == k(); }

	bool invert(const FecKey& k) const { return value == ~k.value; }

private:
	unsigned value;
};

class SimKey
{
public:
	SimKey(CirGate* n, bool inv) {
 		if(inv) value = ~(n->getPattern());
		else value = n->getPattern();
	}
	size_t operator() () const { return value; }
	bool operator == (const SimKey& k) const { return k.value == value; }
private:
	unsigned value;
};
#endif // CIR_GATE_H
