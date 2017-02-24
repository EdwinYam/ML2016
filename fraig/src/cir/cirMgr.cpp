/****************************************************************************
  FileName     [ cirMgr.cpp ]
  PackageName  [ cir ]
  Synopsis     [ Define cir manager functions ]
  Author       [ Chung-Yang (Ric) Huang ]
  Copyright    [ Copyleft(c) 2008-present LaDs(III), GIEE, NTU, Taiwan ]
****************************************************************************/

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctype.h>
#include <cassert>
#include <cstring>
#include <map>
#include "cirMgr.h"
#include "cirGate.h"
#include "util.h"

using namespace std;

// TODO: Implement memeber functions for class CirMgr

/*******************************/
/*   Global variable and enum  */
/*******************************/
CirMgr* cirMgr = 0;

enum CirParseError {
   EXTRA_SPACE,
   MISSING_SPACE,
   ILLEGAL_WSPACE,
   ILLEGAL_NUM,
   ILLEGAL_IDENTIFIER,
   ILLEGAL_SYMBOL_TYPE,
   ILLEGAL_SYMBOL_NAME,
   MISSING_NUM,
   MISSING_IDENTIFIER,
   MISSING_NEWLINE,
   MISSING_DEF,
   CANNOT_INVERTED,
   MAX_LIT_ID,
   REDEF_GATE,
   REDEF_SYMBOLIC_NAME,
   REDEF_CONST,
   NUM_TOO_SMALL,
   NUM_TOO_BIG,

   DUMMY_END
};

/**************************************/
/*   Static varaibles and functions   */
/**************************************/
static unsigned lineNo = 0;  // in printint, lineNo needs to ++
static unsigned colNo  = 0;  // in printing, colNo needs to ++
static string errMsg;
static int errInt;
static CirGate *errGate;

static bool
parseError(CirParseError err)
{
   switch (err) {
	  case EXTRA_SPACE:
		 cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
			  << ": Extra space character is detected!!" << endl;
		 break;
	  case MISSING_SPACE:
		 cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
			  << ": Missing space character!!" << endl;
		 break;
	  case ILLEGAL_WSPACE: // for non-space white space character
		 cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
			  << ": Illegal white space char(" << errInt
			  << ") is detected!!" << endl;
		 break;
	  case ILLEGAL_NUM:
		 cerr << "[ERROR] Line " << lineNo+1 << ": Illegal "
			  << errMsg << "!!" << endl;
		 break;
	  case ILLEGAL_IDENTIFIER:
		 cerr << "[ERROR] Line " << lineNo+1 << ": Illegal identifier \""
			  << errMsg << "\"!!" << endl;
		 break;
	  case ILLEGAL_SYMBOL_TYPE:
		 cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
			  << ": Illegal symbol type (" << errMsg << ")!!" << endl;
		 break;
	  case ILLEGAL_SYMBOL_NAME:
		 cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
			  << ": Symbolic name contains un-printable char(" << errInt
			  << ")!!" << endl;
		 break;
	  case MISSING_NUM:
		 cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
			  << ": Missing " << errMsg << "!!" << endl;
		 break;
	  case MISSING_IDENTIFIER:
		 cerr << "[ERROR] Line " << lineNo+1 << ": Missing \""
			  << errMsg << "\"!!" << endl;
		 break;
	  case MISSING_NEWLINE:
		 cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
			  << ": A new line is expected here!!" << endl;
		 break;
	  case MISSING_DEF:
		 cerr << "[ERROR] Line " << lineNo+1 << ": Missing " << errMsg
			  << " definition!!" << endl;
		 break;
	  case CANNOT_INVERTED:
		 cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
			  << ": " << errMsg << " " << errInt << "(" << errInt/2
			  << ") cannot be inverted!!" << endl;
		 break;
	  case MAX_LIT_ID:
		 cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
			  << ": Literal \"" << errInt << "\" exceeds maximum valid ID!!"
			  << endl;
		 break;
	  case REDEF_GATE:
		 cerr << "[ERROR] Line " << lineNo+1 << ": Literal \"" << errInt
			  << "\" is redefined, previously defined as "
			  << errGate->getTypeStr() << " in line " << errGate->getLineNo()
			  << "!!" << endl;
		 break;
	  case REDEF_SYMBOLIC_NAME:
		 cerr << "[ERROR] Line " << lineNo+1 << ": Symbolic name for \""
			  << errMsg << errInt << "\" is redefined!!" << endl;
		 break;
	  case REDEF_CONST:
		 cerr << "[ERROR] Line " << lineNo+1 << ", Col " << colNo+1
			  << ": Cannot redefine const (" << errInt << ")!!" << endl;
		 break;
	  case NUM_TOO_SMALL:
		 cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
			  << " is too small (" << errInt << ")!!" << endl;
		 break;
	  case NUM_TOO_BIG:
		 cerr << "[ERROR] Line " << lineNo+1 << ": " << errMsg
			  << " is too big (" << errInt << ")!!" << endl;
		 break;
	  default: break;
   }
   return false;
}

/**************************************************************/
/*   class CirMgr constructor/destructor for circuit          */
/**************************************************************/
CirMgr::CirMgr():_sweeped(false),_optimized(false) { _gateList.push_back(new CirCONST()); srand(time(0)); }
CirMgr::~CirMgr() {   
	for(unsigned i = 0 ; i < _gateList.size(); ++i) 
		if(_gateList[i]) delete _gateList[i]; 
}
/**************************************************************/
/*   class CirMgr self defined function for circuit           */
/**************************************************************/
void
CirMgr::findDfs() 
{
	CirGate::setGlobalRef();
	CirGate::setGlobalDFS();
	_dfsList.clear();
	for(unsigned i = _inputNum; i < _inputNum + _outputNum; ++i) {
		_gateList[_idList[i][0]]->partialDFS(_dfsList,false);
	}
}

void
CirMgr::findUndef(){
	for(unsigned i = 0; i < _unDefList.size(); ++i) {
		if(_gateList[_unDefList[i]]) {
			if(_gateList[_unDefList[i]]->_fanOUT.size()==0) {
				delete _gateList[_unDefList[i]];
				_gateList[_unDefList[i]] = 0;
			}
		}
	}
}

void
CirMgr::replace(CirGate*& a, CirGate* b, bool inv)
{
	if (a->isAig()) --_aigNum;
	
	if(b) {
		for(unsigned i = 0; i < a->_fanOUT.size(); ++i) {
			b->_fanOUT.push_back(a->_fanOUT[i]);
			b->_isInvOUT.push_back(inv != a->_isInvOUT[i]);
		}
		for(unsigned i = 0; i < a->_fanOUT.size(); ++i) {
			for(unsigned j = 0; j < a->_fanOUT[i]->_fanIN.size(); ++j) {
				if(a == a->_fanOUT[i]->_fanIN[j]) {
					a->_fanOUT[i]->_fanIN[j] = b;
					a->_fanOUT[i]->_isInvIN[j] = (inv != a->_isInvOUT[i]);
					break;
				}
			}
		}
	}
	delete a;
	a = 0;	
}
/**************************************************************/
/*   class CirMgr member functions for circuit construction   */
/**************************************************************/
bool
CirMgr::readCircuit(const string& fileName)
{
	ifstream file(fileName.c_str(), ios::in);
	if(!file.is_open()) {
		cerr << "Cannot open design \"" << fileName << "\"!!" << endl;
		return false;
	}

	// read in header from files
	
	string header, definition;
	file >> header;
	
	// if (header == "aag") file >> _maxID >> _inputNum >> _latchNum >> _outputNum >> _aigNum;
	if (header == "aag") {
		getline(file, definition);
		colNo = 4;
		if (definition.size()==0) {
			errMsg = "number of varaibles";
			parseError(MISSING_NUM);
			return false;
		}
		assert(definition.find(" ")==0);
		definition = definition.substr(1,string::npos);

		for (unsigned i = 0; i < 5; ++i) {
			int pos = definition.find(" ");
			if ((pos == int(string::npos)) && !(i==4 && definition.size()!=0)) {
				errMsg = "number of varaibles";
				errInt = i;
				parseError(NUM_TOO_SMALL);
				return false;
			}
			colNo = colNo + pos + 1;
			if (pos == 0) { parseError(EXTRA_SPACE); return false; }
			string num = definition.substr(0,pos);

			int temp = 0;
			if (myStr2Int(num,temp)==false) {
				errMsg = num;
				parseError(ILLEGAL_NUM);
				return false;
			}
			if (temp < 0) {
				errMsg = num;
				parseError(ILLEGAL_NUM);
				return false;
			}
			switch (i) {
				case 0: 
					_maxID = temp; break;
				case 1:
					_inputNum = temp; break;
				case 2:
					_latchNum = temp; break;
				case 3:
					_outputNum = temp; break;
				case 4:
					_aigNum = temp; break;
				default:
					break;
			}
			definition = definition.substr(pos+1,string::npos);
		}
	}
	else {
		errMsg = header;
		parseError(ILLEGAL_IDENTIFIER);
		return false; 
	}
	// cout << _maxID << ' ' << _inputNum << ' ' << _outputNum << ' ' << _latchNum << ' ' << _aigNum << endl;
	
	// construct gates
	++lineNo; colNo = 0;
	_gateList.resize(_maxID + _outputNum + 1, 0);
	for(unsigned i = 0; i < _inputNum; ++i, ++lineNo) {
		unsigned inputID;
		file >> inputID;
		// cout << "line" << lineNo+1 << ',' << i+2 <<':'<< inputID << endl;
		IdList temp;
		temp.push_back(inputID/2);
		_idList.push_back(temp);
		_gateList[inputID/2] = new CirPI(inputID/2,i + 2);
	}
	for(unsigned i = 0; i < _outputNum; ++i, ++lineNo) {
		unsigned outputID;
		file >> outputID;
		// cout << "line" << lineNo+1 << ',' << i+_inputNum+2 <<':'<< outputID << endl;
		IdList temp;
		temp.push_back(_maxID + i + 1);
		temp.push_back(outputID);
		_idList.push_back(temp);
		_gateList[_maxID + i + 1] = new CirPO(_maxID + i + 1, i+_inputNum+_latchNum+2);
	}
	for(size_t i = 0; i < _aigNum; ++i, ++lineNo) {
		unsigned out, in1, in2;
		file >> out >> in1 >> in2;
		// cout << "line" << lineNo+1 << ',' << i+_inputNum+_outputNum+2 <<':'<< out << endl;
		IdList temp;
		temp.push_back(out/2);
		temp.push_back(in1);
		temp.push_back(in2);
		_idList.push_back(temp);
		_gateList[out/2] = new CirAIG(out/2, i+_inputNum+_latchNum+_outputNum+2);
	}
	// cout << "gateNUM: " << _gateList.size() << endl;
	// cout << "Linking..." << endl;

	for(unsigned i = _inputNum + _outputNum; i < _idList.size(); ++i) {
		CirGate* gate = _gateList[ _idList[i][0] ];
		if(gate == 0) continue;
		if(!(gate->isAig())) continue;
		CirGate* in1 = _gateList[ _idList[i][1] / 2 ];
		CirGate* in2 = _gateList[ _idList[i][2] / 2 ];
		if(in1 == 0) {
			in1 = new CirUNDEF(_idList[i][1] / 2);
			_unDefList.push_back(_idList[i][1] / 2);
			_gateList[_idList[i][1] / 2] = in1;
		}
		if(in2 == 0) {
			in2 = new CirUNDEF(_idList[i][2] / 2);
			_unDefList.push_back(_idList[i][2] / 2);
			_gateList[_idList[i][2] / 2] = in2;
		}

		gate->linkFanInandOut(in1, bool(_idList[i][1] % 2));
		gate->linkFanInandOut(in2, bool(_idList[i][2] % 2));
	}
	
	for(unsigned i = _inputNum; i < _inputNum + _outputNum; ++i) {
		CirGate* gate = _gateList[ _idList[i][0] ];
		if(gate == 0) continue;
		if(!(gate->isPO())) continue;

		CirGate* input = _gateList[ _idList[i][1] / 2 ];
		if(input == 0) {
			input = new CirUNDEF(_idList[i][1] / 2);
			_unDefList.push_back(_idList[i][1] / 2);
			_gateList[_idList[i][1] / 2] = input;
		}
		gate->linkFanInandOut(input, bool(_idList[i][1] % 2));
	}
	file.ignore();
  
	while (file.peek() != 'c' && file.peek() != EOF) {
		++lineNo;
		string setSymbolDef;
		getline(file, setSymbolDef);
		if (setSymbolDef.empty()) {
			cerr << "Unexpected emtpy line occurrs at line " << lineNo << "!!" << endl;
			return false;
		}
		string def, symbol;
		unsigned pos = setSymbolDef.find(" ");
		def = setSymbolDef.substr(0,pos);
		if (def.empty()) {
			cerr << "Unexpected emtpy line occurrs at line " << lineNo << "!!" << endl;
			return false;
		}
		symbol = setSymbolDef.substr(pos+1,string::npos);
		if (symbol.empty()) {
			errMsg = "symbol name";
			parseError(MISSING_IDENTIFIER);
			return false;
		}
		char io = def[0];
		def = def.substr(1,string::npos);
		int gateSequenceNum;
		if(def.size()) myStr2Int(def, gateSequenceNum);
		else return false;
		if(io == 'i') {
			_gateList[gateSequenceNum + 1]->setSymbolStr(symbol);
		}
		else if(io == 'o') {
			_gateList[_maxID + 1 + gateSequenceNum]->setSymbolStr(symbol);
		}
	}
	lineNo = 0;
	findDfs();
	return true;
}

/**********************************************************/
/*   class CirMgr member functions for circuit printing   */
/**********************************************************/
/*********************
Circuit Statistics
==================
  PI          20
  PO          12
  AIG        130
------------------
  Total      162
*********************/
void
CirMgr::printSummary() const
{
	cout << endl;
	cout << "Circuit Statistics" << endl;
	cout << "==================" << endl;
	cout << "  " << setw(8) << left << "PI" << setw(8) << right << _inputNum << endl
		<< "  " << setw(8) << left << "PO" << setw(8) << right << _outputNum << endl
		<< "  " << setw(8) << left << "AIG" << setw(8) << right << _aigNum << endl
		<< "------------------" << endl
		<< "  " << setw(8) << left << "Total" << setw(8) << right << _inputNum+_outputNum+_aigNum << endl;
}

void
CirMgr::printNetlist() const
{
	
	cout << endl;
	for (unsigned i = 0, num = 0, n = _dfsList.size(); i < n; ++i) {
		if (!(_gateList[_dfsList[i]]->isUNDEF())) { cout << "[" << num << "] "; ++num; }
		_gateList[_dfsList[i]]->printGate();
		
	}
	/*
	CirGate::setGlobalRef();
	unsigned index = 0;
	cout << endl;
	for(unsigned i = 0; i < _gateList.size(); ++i) {
		if (_gateList[i] == 0) continue;
		if (_gateList[i]->isPO()) _gateList[i]->printDFS(index);
	}
	*/
}

void
CirMgr::printPIs() const
{
	cout << "PIs of the circuit: ";
	for (unsigned i = 0; i < _inputNum; ++i){
		cout << _idList[i][0] << ' ';
	}
	cout << '\b' << endl;
}

void
CirMgr::printPOs() const
{
	cout << "POs of the circuit: ";
	for (unsigned i = _inputNum; i < _inputNum+_outputNum; ++i){
		cout << _idList[i][0] << ' ';
	} 
	cout << '\b' << endl;
}

void
CirMgr::printFloatGates() const
{
	IdList floatGates, unUsedGates, temp;
	for(unsigned i = 0; i < _gateList.size(); ++i) {
		if (_gateList[i] == 0) continue;
		if (_gateList[i]->isUNDEF()) {
			temp = _gateList[i]->getFanOutID();
			floatGates.insert(floatGates.end(), temp.begin(), temp.end());
		}
		if (_gateList[i]->isPO() || _gateList[i]->isCONST()) continue;
		temp = _gateList[i]->getFanOutID();
		if (temp.size()==0) {
			unUsedGates.push_back(_gateList[i]->getID());
		}
	}
	sort(floatGates.begin(), floatGates.end());
	sort(unUsedGates.begin(), unUsedGates.end());
	if(floatGates.size()) {
		cout << "Gates with floating fanin(s):";
		for(unsigned i = 0; i < floatGates.size(); ++i)
			cout << " " << floatGates[i];
		cout << endl;
	}
	if(unUsedGates.size()) {
		cout << "Gates defined but not used  :";
			for(unsigned i = 0; i < unUsedGates.size(); ++i)
				cout << " " << unUsedGates[i];
		cout << endl;
	}
}

void
CirMgr::printFECPairs() const
{
	for(unsigned i = 0; i < _fecGroupList.size(); ++i) {
		cout << "[" << i << "]";
		for(unsigned j = 0; j < _fecGroupList[i]->size(); ++j)
			cout << " " << ((*_fecGroupList[i])[j] % 2 ? "!" : "") << (*_fecGroupList[i])[j] / 2;
		cout << endl;
	}
}

void
CirMgr::writeAag(ostream& outfile) const
{
	CirGate::setGlobalRef();
	IdList writeAIGList;
	for(unsigned i = 0; i < _gateList.size(); ++i) {
		if (_gateList[i] == 0) continue;
		if (_gateList[i]->isPO()) 
			_gateList[i]->partialDFS(writeAIGList,true);
	}
	outfile << "aag" << " " << _maxID 
				   << " " << _inputNum 
				   << " " << _latchNum 
				   << " " << _outputNum 
				   << " " << writeAIGList.size() << endl;
	for(unsigned i = 0; i < _inputNum; ++i)
		outfile << _idList[i][0]*2 << endl;
	for(unsigned i = _inputNum; i < _inputNum + _outputNum; ++i)
		outfile << _idList[i][1] << endl;
	for(unsigned i = 0; i < writeAIGList.size(); ++i) {
		outfile << _idList[writeAIGList[i]][0]*2 << " " 
				<< _idList[writeAIGList[i]][1] << " " 
				<< _idList[writeAIGList[i]][2] << endl;
	}
	unsigned idx1 = 0, idx2 = 0;
	for(unsigned i = 0; i < _gateList.size(); ++i) {
		if (_gateList[i] == 0) continue;
		if (_gateList[i]->isPI()) {
			string symbol = _gateList[i]->getSymbolStr();
			if(symbol.size()) outfile << "i" << idx1 << " " << symbol << endl;
			idx1++;
		}
		else if (_gateList[i]->isPO()) {
			string symbol = _gateList[i]->getSymbolStr();
			if(symbol.size()) outfile << "o" << idx2 << " " << symbol << endl;
			idx2++;
		}
	}
}

void
CirMgr::writeGate(ostream& outfile, CirGate *g) const
{

	IdList _partialList, _aigList, _piList;
	CirGate::setGlobalRef();
	g->partialDFS(_partialList,false);
	CirGate::setGlobalRef();
	g->partialDFS(_aigList,true);
	for(unsigned i = 0; i < _partialList.size(); ++i) {
		if(_gateList[_partialList[i]]->isPI())
	 		_piList.push_back(_partialList[i]);
	}
	sort(_piList.begin(), _piList.end());
	outfile << "aag" << " " << g->_id << " " << _piList.size() << " " << _latchNum << " " << 1 << " " << _aigList.size() << endl;
	for(unsigned n = 0; n < _piList.size(); ++n)
		outfile << _piList[n] * 2 << endl;
	outfile << g->_id * 2 << endl;
	for(unsigned n = 0; n < _aigList.size(); ++n) {
		outfile 
		<< _idList[_aigList[n]][0]*2 << " "
		<< _idList[_aigList[n]][1] << " "
		<< _idList[_aigList[n]][2]
		<< endl;
	}
	for(unsigned i = 0; i < _piList.size(); ++i) {
		string symbol = _gateList[_piList[i]]->getSymbolStr();
		if(symbol.size()) outfile << "i" << i << " " << symbol << endl;
	}
	string symbol = g->getSymbolStr();
	outfile << "o" << 0 << " " << g->_id << " " << symbol << endl;
	
}

