/****************************************************************************
  FileName     [ cirGate.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define class CirAigGate member functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdarg.h>
#include <cassert>
#include "cirGate.h"
#include "cirMgr.h"
#include "util.h"

using namespace std;

// TODO: Keep "CirGate::reportGate()", "CirGate::reportFanin()" and
//       "CirGate::reportFanout()" for cir cmds. Feel free to define
//       your own variables and functions.

extern CirMgr *cirMgr;
/**************************************/
/*   class CirGate constructor        */
/**************************************/
CirGate::~CirGate()
{
	for(unsigned i = 0; i < _fanIN.size(); ++i) {
		for(unsigned j = 0; j < _fanIN[i]->_fanOUT.size(); ++j) {
			if(this == _fanIN[i]->_fanOUT[j]) {
				_fanIN[i]->_fanOUT.erase(_fanIN[i]->_fanOUT.begin() + j);
				_fanIN[i]->_isInvOUT.erase(_fanIN[i]->_isInvOUT.begin() + j);
				break;
			}
		}
	}

	for(unsigned i = 0; i < _fanOUT.size(); ++i) {
		for(unsigned j = 0; j < _fanOUT[i]->_fanIN.size(); ++j) {
			if(this == _fanOUT[i]->_fanIN[j]) {
				_fanOUT[i]->_fanIN.erase(_fanOUT[i]->_fanIN.begin() + j);
				_fanOUT[i]->_isInvIN.erase(_fanOUT[i]->_isInvIN.begin() + j);
				break;
			}
		}
	}
}

bool
CirPO::simulateforGate(bool init)
{
	if(init) {
		_pattern = _fanIN[0]->getPattern() ^ (0 - _isInvIN[0]);
		return true;
	}
	if(_fanIN[0]->simulateforGate(init)) {
		_prePattern = _pattern;
		_pattern = _fanIN[0]->getPattern() ^ (0 - _isInvIN[0]);
	}
	return _pattern != _prePattern;
}

bool
CirAIG::simulateforGate(bool init)
{
	if(init) {
		_pattern = _fanIN[0]->getPattern() ^ (0 - _isInvIN[0]);
		return true;
	}

	if(isGlobalState()) return _pattern != _prePattern;
	mark2GlobalState();
	if( (_fanIN[0]->simulateforGate(init)) 
		&& (_fanIN[1]->simulateforGate(init)) ) 
	{
		size_t newPattern;
		newPattern = (_fanIN[0]->getPattern() ^ (0 - _isInvIN[0]) ) 
					& (_fanIN[1]->getPattern() ^ (0 - _isInvIN[1]) );
		_prePattern = _pattern;
		_pattern = newPattern;
	}
	return _pattern != _prePattern;
}
/**************************************/
/*   class CirGate member functions   */
/**************************************/
unsigned CirGate::_globalState = 1;
unsigned CirGate::_dfsState = 1;
void 
CirGate::printGate() const
{
	cout << setw(4) << left << getTypeStr() << _id;
	for(unsigned n = 0; n < _fanIN.size(); ++n) {
		cout << " ";
		if(_fanIN[n]->isUNDEF()) cout << "*";
		if(_isInvIN[n]) cout << "!";
		cout << _fanIN[n]->_id;
	}
	if(_symbol.size()) cout << " (" << _symbol << ")";
	cout << endl;
}
void
CirGate::reportGate() const
{
	stringstream buffer;
	cout << "==================================================" << endl;
	buffer << "= " << this->getTypeStr() << "(" << _id << ")";
	if(_symbol.size()) buffer << "\"" << _symbol << "\"";
	buffer << ", line " << _lineNo;
	string s = buffer.str();
	s.resize(49, ' ');
	s += "=";
	cout << s << endl << "==================================================" << endl;
}

void
CirGate::reportFanin(int level) const
{
   assert (level >= 0);
   setGlobalRef();
   depthFirstSearch(level,false,0);
}

void
CirGate::reportFanout(int level) const
{
   assert (level >= 0);
   setGlobalRef();
   depthFirstSearch(level,true,0);
}

void 
CirGate::depthFirstSearch(int level, bool downWard, int currentlevel) const
{
	assert (level >= 0);
	if (currentlevel > level) return;
	if (currentlevel == 0) { cout << this->getTypeStr() << ' ' << _id << endl; currentlevel++; }
	mark2GlobalState();
	const GateList dfsList = downWard?(_fanOUT):(_fanIN);
	if (dfsList.size()==0) return; // how about level is over
	const vector<bool> isInv = downWard?(_isInvOUT):(_isInvIN);
	for (unsigned i = 0; i < dfsList.size(); ++i) {
		for (unsigned j = 0; j < unsigned(currentlevel); ++j) cout << "  ";
		if (isInv[i]) cout << '!';
		cout << dfsList[i]->getTypeStr() << ' ' << dfsList[i]->_id;
		if (dfsList[i]->isGlobalState()) cout << " (*)" << endl; 
		else {
			cout << endl;
			dfsList[i]->depthFirstSearch(level, downWard, currentlevel+1);
		}
	}
}

// self defined function
void 
CirGate::linkFanInandOut(CirGate* in, bool isInv) 
{
	assert(in != 0);
	_fanIN.push_back(in);
	_isInvIN.push_back(isInv);
	in->_fanOUT.push_back(this);
   	in->_isInvOUT.push_back(isInv);
}
void
CirGate::partialDFS(IdList& list, bool isPartial) const
{
	for(unsigned i = 0; i < _fanIN.size(); ++i)
		if(!_fanIN[i]->isGlobalState()) { _fanIN[i]->partialDFS(list,isPartial); }
	if (isPartial) {
		mark2GlobalState();
		if(this->isAig()) list.push_back(_lineNo - 2);
	}
	else {
		mark2GlobalState(true); 
		list.push_back(_id);
	}
}

void
CirGate::printDFS(unsigned& index)
{
	if(this->isUNDEF()) return;
	for(unsigned i = 0; i < unsigned(_fanIN.size()); ++i)
		if(_fanIN[i]->isGlobalState() == false) { _fanIN[i]->printDFS(index); }
	cout << "[" << index << "] " << setw(5) << left << this->getTypeStr() << _id;
	for(unsigned i = 0; i < _fanIN.size(); ++i) {
		cout << ' ';
		if(_fanIN[i]->isUNDEF()) cout << '*';
		if(_isInvIN[i]) cout << '!';
		cout << _fanIN[i]->_id;     
	}
	if(_symbol.size()) cout << " (" << _symbol << ")";
	cout << endl;
	++index;
	mark2GlobalState();
}
IdList
CirGate::getFanOutID() const
{
	IdList fanOutID;
   	for(unsigned i = 0; i < unsigned(_fanOUT.size()); ++i)
		fanOutID.push_back(_fanOUT[i]->getID());
	return fanOutID; 
}

IdList
CirGate::getFanInID() const
{
	IdList fanInID;
   	for(unsigned i = 0; i < unsigned(_fanIN.size()); ++i)
		fanInID.push_back(_fanIN[i]->getID());
	return fanInID; 
}