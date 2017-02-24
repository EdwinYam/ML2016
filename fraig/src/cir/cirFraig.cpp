/****************************************************************************
  FileName     [ cirFraig.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir FRAIG functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2012-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <cassert>
#include "cirMgr.h"
#include "cirGate.h"
#include "sat.h"
#include "myHashMap.h"
#include "util.h"

using namespace std;

// TODO: Please keep "CirMgr::strash()" and "CirMgr::fraig()" for cir cmd.
//       Feel free to define your own variables or functions

/*******************************/
/*   Global variable and enum  */
/*******************************/

/**************************************/
/*   Static varaibles and functions   */
/**************************************/

/*******************************************/
/*   Public member functions about fraig   */
/*******************************************/
// _floatList may be changed.
// _unusedList and _undefList won't be changed
void
CirMgr::strash()
{
	HashMap<HashKey, CirGate*> hashGate(getHashSize(_aigNum));
	for(unsigned i = 0; i < _dfsList.size(); ++i) {
		CirGate*& cur = _gateList[_dfsList[i]];
		if(cur->isAig()) {
			unsigned a = 2 * cur->_fanIN[0]->_id + cur->_isInvIN[0];
			unsigned b = 2 * cur->_fanIN[1]->_id + cur->_isInvIN[1];
			if (a > b) { unsigned temp = a; a = b; b = temp; }
			HashKey k(a,b);
			if(!hashGate.insert(k, cur)) {
				CirGate* pre = 0;
				hashGate.query(k, pre);
				cout << "Strashing: " << pre->_id << " merging " << cur->_id << "..." << endl;
				replace(cur, pre);
			}
		}
	}
	findDfs();
}

void
CirMgr::fraig()
{
	
}

/********************************************/
/*   Private member functions about fraig   */
/********************************************/
