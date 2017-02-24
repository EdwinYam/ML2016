/****************************************************************************
  FileName     [ cirMgr.h ]
  PackageName  [ cir ]
  Synopsis     [ Define circuit manager ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#ifndef CIR_MGR_H
#define CIR_MGR_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;

// TODO: Feel free to define your own classes, variables, or functions.

#include "cirDef.h"

extern CirMgr *cirMgr;

class CirMgr
{
public:
	CirMgr();
	~CirMgr(); 

	// Access functions
	// return '0' if "gid" corresponds to an undefined gate.
	CirGate* getGate(unsigned gid) const { if(gid>_maxID+_outputNum) return 0; return _gateList[gid]; }

	// Member functions about circuit construction
	bool readCircuit(const string&);

	// Member functions about circuit optimization
	void sweep();
	void optimize();
	void replace(CirGate*& a, CirGate* b = 0, bool inv = false);

	// Member functions about simulation
	void randomSim();
	void fileSim(ifstream&);
	void setSimLog(ofstream *logFile) { _simLog = logFile; }
	void initialization(simList&);
	void createFecList();
	void reFecList();
	void deleteFEC();

	// Member functions about fraig
	void strash();
	void printFEC() const;
	void fraig();

	// Member functions about circuit reporting
	void printSummary() const;
	void printNetlist() const;
	void printPIs() const;
	void printPOs() const;
	void printFloatGates() const;
	void printFECPairs() const;
	void writeAag(ostream&) const;
	void writeGate(ostream&, CirGate*) const;

	// Self defined function
	void findDfs();
	void findUndef();
	void setMaxID();
private:
	ofstream           *_simLog;
	bool _sweeped, _optimized;
	unsigned _maxID, _inputNum, _latchNum, _outputNum, _aigNum;
	GateList _gateList;
	IdList _unDefList, _dfsList;
	vector<IdList> _idList;
	vector<IdList*> _fecGroupList;
};

#endif // CIR_MGR_H
