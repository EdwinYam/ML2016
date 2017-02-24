/****************************************************************************
  FileName     [ cirSim.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir optimization functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::sweep()" and "CirMgr::optimize()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
void
CirMgr::setMaxID() 
{
	if (_gateList[_maxID]!=0) return;
	_maxID = 0;
	for(unsigned i = 0; i<_dfsList.size(); ++i) {
		if (_gateList[_dfsList[i]]->_id > _maxID) _maxID = _gateList[_dfsList[i]]->_id;
	}
}
/**************************************************/
/*   Public member functions about optimization   */
/**************************************************/
// Remove unused gates
// DFS list should NOT be changed
// UNDEF, float and unused list may be changed
void
CirMgr::sweep()
{
	if(_sweeped) return;
	for(unsigned i = 1; i < _gateList.size(); ++i) {
		if(_gateList[i]==0) continue;
		if(!(_gateList[i]->inDFSList())) {
			if(_gateList[i]->isAig() || _gateList[i]->isUNDEF()) {
				cout << "Sweeping: " << _gateList[i]->getTypeStr()
					 << "(" << _gateList[i]->getID() << ") removed..." << endl;
				if (_gateList[i]->isAig()) --_aigNum;
				delete _gateList[i];
				_gateList[i] = 0;
			}
		}
	}
	setMaxID();
	findUndef();
	_sweeped = true;
}

// Recursively simplifying from POs;
// _dfsList needs to be reconstructed afterwards
// UNDEF gates may be delete if its fanout becomes empty...
void
CirMgr::optimize()
{
	if (_optimized) return;
	for(unsigned i = 0; i < _dfsList.size(); ++i) {
		CirGate *&currentGate = _gateList[_dfsList[i]], *pre = 0;
		bool invOrNot = false;
		if(!(currentGate->isAig())) continue;

		// Identical gate
		if(currentGate->_fanIN[0] == currentGate->_fanIN[1]) {
			// Identical
			if(currentGate->_isInvIN[0] == currentGate->_isInvIN[1])
				{ pre = currentGate->_fanIN[0]; invOrNot = currentGate->_isInvIN[0]; }
			// Inverted
			else { pre = _gateList[0]; invOrNot = false; }
		}
		else {
			for(unsigned j = 0; j < 2; ++j) {
				// Fanin const 1
				if((currentGate->_fanIN[j]->isCONST() && currentGate->_isInvIN[j]))
				{ pre = currentGate->_fanIN[1-j]; invOrNot = currentGate->_isInvIN[1-j]; break; }
				// Fanin const 0
				else if((currentGate->_fanIN[j]->isCONST()) && !(currentGate->_isInvIN[j]))
				{ pre = _gateList[0]; invOrNot = false; break;}
			}
		}
		if(pre) {
			cout << "Simplifying: " << pre->_id << " merging " << (invOrNot ? "!" : "") << currentGate->_id << "..." << endl;
			replace(currentGate, pre, invOrNot);
		}
	}
	findDfs();
	findUndef();
	setMaxID();
	_sweeped = false;
	_optimized = true;
}


/***************************************************/
/*   Private member functions about optimization   */
/***************************************************/
