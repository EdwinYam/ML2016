/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir simulation functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <cmath>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"
#include "myHashMap.h"
using namespace std;

// TODO: Keep "CirMgr::randimSim()" and "CirMgr::fileSim()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/************************************************/
/*   Public member functions about Simulation   */
/************************************************/
void
CirMgr::randomSim()
{
	unsigned maxFails = log(_dfsList.size()) , failure = 0, simNum = 0, lastFecSize = 0;
	simList simPattern(_inputNum);
	cout << "MAX_FAILS = " << maxFails << endl;

	while(failure < maxFails){
		for(unsigned i = 0; i < _inputNum; ++i) simPattern[i] = rand();
		initialization(simPattern);
		simNum += 8;
		
		if(_fecGroupList.size()==0) { 
			cout << "First Constructing FecGroup..." << endl; 
			createFecList();
		}
		else reFecList();
		deleteFEC();
		if(lastFecSize == _fecGroupList.size()) ++failure;
		else failure = 0;
		lastFecSize = _fecGroupList.size();
	}
	cout << sizeof(size_t)*simNum << " patterns simulated." << endl;
}

void
CirMgr::fileSim(ifstream& patternFile)
{
	unsigned num = 0;
	simList simPattern(_inputNum, 0);
	string line;
	while(!patternFile.eof()) {
		patternFile >> line;
		while(patternFile.peek() == '\n' || patternFile.peek() == ' ') patternFile.get();
		++num;
		if(line.size() != _inputNum) {
			cout << "Error: Pattern(" << line <<") length(" << line.size()
				<< ") does not match the number of inputs(" << _inputNum << ") in a circuit!!" << endl;
		}
		for(unsigned i = 0; i < _inputNum; ++i) 
			simPattern[i] = (simPattern[i] << 1) + line[i] - 48;

		if((num % (4*sizeof(size_t))) == 0) {
			initialization(simPattern);
			if (_fecGroupList.size()==0) {
				cout << "First Constructing FecGroup..." << endl;
				createFecList();
			}
			else {
				reFecList(); // simLog();
			}
			deleteFEC();
			simList temp(_inputNum, 0);
			simPattern.swap(temp);
		}
	}
	if((num % (4*sizeof(size_t))) != 0) {
		initialization(simPattern);
		if (_fecGroupList.size()==0) {
			cout << "First Constructing FecGroup..." << endl;
			createFecList();
		}
		else {
			reFecList(); // simLog(num % (4*sizeof(size_t))); 
		}
		deleteFEC();
	}
	cout << num << " patterns simulated." << endl;
}

/*************************************************/
/*   Private member functions about Simulation   */
/*************************************************/
void
CirMgr::initialization(simList& simPattern) 
{
	for(unsigned i = 0; i < _inputNum; ++i) {
		static_cast<CirPI*>(_gateList[_idList[i][0]]) -> setPattern(simPattern[i]);
	}
}
void
CirMgr::createFecList()
{
	IdList aigDFSList;
	aigDFSList.push_back(0);
	for(unsigned i = 0; i < _dfsList.size(); ++i) {
		if(_gateList[_dfsList[i]]->isAig()) {
			static_cast<CirAIG*>(_gateList[_dfsList[i]]) -> simulateforGate(true);
			aigDFSList.push_back(_dfsList[i]);
		}
	}
	sort(aigDFSList.begin(), aigDFSList.end());

	HashMap<FecKey, IdList*> fecHash(getHashSize(aigDFSList.size()));
	for(unsigned n = 0; n < aigDFSList.size(); ++n) {
		FecKey key(_gateList[aigDFSList[n]]);
		IdList* u;
		// Find new group
		if(!fecHash.query(key, u)) {
			u = new IdList(); 
			_fecGroupList.push_back(u);

			u->push_back(aigDFSList[n] * 2 );
			
			// Update fecList and gate fec info
			fecHash.insert(key, u);
		}
		// Insert to existing group
		else {
			u->push_back(aigDFSList[n] * 2 + fecHash.check(key,true));
		}
	}
}

void
CirMgr::reFecList()
{
	// Simulate (event-driven)
	CirGate *temp = new CirUNDEF(0);
	temp->mark2GlobalState();
	delete temp;

	for(unsigned i = _maxID; i < _maxID + _outputNum; ++i) {
		_gateList[_idList[i][0]] -> simulateforGate(false);
	}
	// Find FEC
	vector<IdList*> newList;
	for(unsigned i = 0, s = _fecGroupList.size(); i < s; ++i) {
		HashMap<SimKey, IdList*> fecHash( getHashSize(_fecGroupList[i]->size() ) );
		for(unsigned j = 0; j < _fecGroupList[i]->size(); ++j) {
			unsigned cur = (*_fecGroupList[i])[j];
			SimKey key(_gateList[cur / 2], cur % 2);
			IdList* u;
			// Find new group
			if(!fecHash.query(key, u)) {
				u = new IdList(); 
				newList.push_back(u);
				u->push_back(cur); 
				
				fecHash.insert(key, u);
			}
			// Insert to existing group
			else {
				u->push_back(cur);
			}
		 }
	}
	for(unsigned n = 0; n < _fecGroupList.size(); ++n)
		delete _fecGroupList[n];
	_fecGroupList.swap(newList);
}
void
CirMgr::deleteFEC(){
	for(int n = _fecGroupList.size() - 1; n >= 0 ; --n) {
		if(_fecGroupList[n]->size() == 1) { 
			delete _fecGroupList[n]; 
			_fecGroupList.erase(_fecGroupList.begin() + n);
		}
	}
	cout << "Total #FEC Group = " << _fecGroupList.size() << char(13) << flush;
}
/*
void
CirMgr::simLog(unsigned mask)
{
	if(!_simLog) return;
	if(mask == 0) mask = (1 << 31);
	else mask = (1 << (mask - 1));
	for(unsigned i = 0; i < _inputNum; ++i)
		*_simLog << (bool)(_gateList[_idList[i][0]]->_pattern & mask);
	*_simLog << " ";
	for(unsigned i = _maxID; i < _maxID + _outputNum; ++i)
		*_simLog << (bool)(_gateList[_idList[i][0]]->_pattern & mask);
	*_simLog << endl;
	mask >>= 1;
}
*/