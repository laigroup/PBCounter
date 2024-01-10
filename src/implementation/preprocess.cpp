/* inclusions *****************************************************************/

#include "../interface/preprocess.hpp"

/* class Preprocessor *********************************************************/

string getConstraintRelation(ConstraintRelation constraintRelation) {
    switch (constraintRelation) {
        case ConstraintRelation::EQUAL: {
            return "EQUAL";
        }
        case ConstraintRelation::GEQUAL: {
            return "GEQUAL";
        }
        case ConstraintRelation::LEQUAL: {
            return "LEQUAL";
        }
        case ConstraintRelation::_EQUAL: {
            return "_EQUAL";
        }
        case ConstraintRelation::_GEQUAL: {
            return "_GEQUAL";
        }
        case ConstraintRelation::_LEQUAL: {
            return "_LEQUAL";
        }
        case ConstraintRelation::NEQUAL: {
            return "NEQUAL";
        }
    }
}

Int Preprocessor::getCoefSum(Int consIndex) const {
    // Int sum = 0, vecSize = _literals[consIndex].size();
    Int sum = 0;
    for (const Int& lit : _literals[consIndex]) {

        if (_delLiterals[consIndex].count(lit) > 0)  continue;

        if (_coefficients[consIndex].at(lit) <= 0) {
            showError("Wrong coefficient value");
        } else {
            sum += _coefficients[consIndex].at(lit);
        }
    }
    return sum;
}

Int Preprocessor::getCoefSum(const vector<Int> &coefficient) const {
    Int sum = 0, vecSize = coefficient.size();
    for (Int i = 0; i < vecSize; i++) {
        if (coefficient[i] <= 0) showError("Wrong coefficient value");
        else sum += coefficient[i];
    }
    return sum;
}

Int Preprocessor::findVar(const vector<Int> &literal, const Int& var) {
    for (Int i = 0; i < literal.size(); i++) {
        if (literal[i] == var || literal[i] == -var) return i;
        if (std::abs(literal[i]) > std::abs(var)) return -1;     // sorted constraint
    }
    return -1;
}

void Preprocessor::removeConstraint(Int consIndex) {
    _literals.erase(_literals.begin() + consIndex);
    _coefficients.erase(_coefficients.begin() + consIndex);

    _upperBound.erase(_upperBound.begin() + consIndex);
    _lowerBound.erase(_lowerBound.begin() + consIndex);
    _options.erase(_options.begin() + consIndex);

    _curCoefSum.erase(_curCoefSum.begin() + consIndex);
    _curCoefVal.erase(_curCoefVal.begin() + consIndex);
    _delLiterals.erase(_delLiterals.begin() + consIndex);

    _constraintCnt--;

    judgeSize();
}

void Preprocessor::removeTruthConstraints() {
    for(Int consIndex = 0;  consIndex < _constraintCnt; consIndex++) {
        if (judgeTruthConstraint(consIndex)) {  // This constraint consIndexs always ture
            removeConstraint(consIndex);
            consIndex--;
        }
    }
}

bool Preprocessor::judgeTruthConstraint(Int consIndex) const {
    // Int lbVal = 0, ubVal = _curCoefSum[consIndex];
    // Int curVal = _curCoefVal[consIndex];
    // curVal + lbVal >= lb   && curVal + ubVal <= ub

    // printConstraint(consIndex);
    // cout << "Val: " << _curCoefVal[consIndex] << " Sum: " << _curCoefSum[consIndex] << std::endl;
    
    return (_curCoefVal[consIndex] + 0 >= _lowerBound[consIndex] 
            && _curCoefVal[consIndex] + _curCoefSum[consIndex] <= _upperBound[consIndex]);
}

void Preprocessor::delLitFromCons(Int consIndex, Int lit, bool trueAssign) {
    // cout << "Delete " << lit << " from " << consIndex << endl;
    Int coef = _coefficients[consIndex][lit];
    _curCoefSum[consIndex] -= coef;
    _curCoefVal[consIndex] += trueAssign ? coef : 0;
    _delLiterals[consIndex].insert(lit);
}

void Preprocessor::rstLitFromCons(Int consIndex, Int lit, bool trueAssign) {
    assert(_delLiterals[consIndex].count(lit) > 0);
    // cout << "Reset " << lit << " from " << consIndex << endl;
    Int coef = _coefficients[consIndex][lit];
    _curCoefSum[consIndex] += coef;
    _curCoefVal[consIndex] -= trueAssign ? coef : 0;
    // _delLiterals[consIndex].erase(lit);
    // !!! erase out of this funtion
}

void Preprocessor::printConstraint(Int consIndex) const {
    cout << std::setw(5) << _lowerBound[consIndex] << " <= ";
    for (const Int& lit : _literals[consIndex]) {
        cout << std::right << std::setw(5) << ("+" + to_string(_coefficients[consIndex].at(lit))) << " x";
        cout << std::left  << std::setw(5) << lit << " ";
    }
    cout << std::right << std::setw(5) << " <= " << _upperBound[consIndex];
    cout << std::endl;
}

void Preprocessor::printFormula() const {
    for (Int consIndex = 0; consIndex < _constraintCnt; consIndex++) {
        cout << "cons " << consIndex << ":\t";
        printConstraint(consIndex);
    }
}

ConstraintRelation Preprocessor::judgeConstraints(const Set<Int>& literal, const Map<Int, Int>& coefficient, const Set<Int>& _literal, const Map<Int, Int>& _coefficient) const {
    ConstraintRelation relation = ConstraintRelation::EQUAL;
    Int reverseFlag = -1;       // -1 unknown, 0 No reverse, 1 reverse

    Int eqCnt = 0, geqCnt = 0;
    for (Int lit : literal) {
        if (_literal.count(lit) > 0) {
            if (reverseFlag == -1) reverseFlag = 0;
            else if (reverseFlag == 1) return ConstraintRelation::NEQUAL;

            // only consider same coefTerm
            if (coefficient.at(lit) != _coefficient.at(lit)) return ConstraintRelation::NEQUAL;
            eqCnt++;
        }
        else if (_literal.count(-lit) > 0) {
            if (reverseFlag == -1) reverseFlag = 0;
            else if (reverseFlag == 0) return ConstraintRelation::NEQUAL;

            if (coefficient.at(lit) != _coefficient.at(-lit)) return ConstraintRelation::NEQUAL;
            eqCnt ++;
        }
        else geqCnt++;
    }

    if (geqCnt == 0) {  // all A in B
        if (literal.size() == _literal.size()) return ConstraintRelation::EQUAL;
        if (literal.size() <  _literal.size()) return reverseFlag ? ConstraintRelation::_LEQUAL : ConstraintRelation::LEQUAL;
        else util::showError("Error in judgeConstraints with geqCnt = 0");
    }
    else {              // not all A in B
        if (_literal.size() == eqCnt) return reverseFlag ? ConstraintRelation::_GEQUAL : ConstraintRelation::GEQUAL;
        else return ConstraintRelation::NEQUAL;
    }
}

// judge if only one Model,  false - more than one model or no model
bool Preprocessor::getOneModel(Int consIndex, vector<Int> &model) const {
    vector<Int> litVec;

    for (Int lit : _literals[consIndex]) litVec.push_back(lit);

    Int sz = litVec.size(), now = 0;

    if (sz > 10) return false;
    else if (sz == 0) showError("Wrong constraint when getModel");

    model.clear();

    while(now < (1 << sz)) {
        Int sum = 0;
        for (Int i = 0; i < sz; i++) {
            if ((now >> i) & 1) sum += _coefficients[consIndex].at(litVec[i]);
        }
        if (sum >= _lowerBound[consIndex] && sum <= _upperBound[consIndex]) {
            if (model.empty()) {
                for (Int i = 0; i < sz; i++) {
                    if ((now >> i) & 1) model.push_back(litVec[i]);
                    else model.push_back(-litVec[i]);
                }
            } else {
                model.clear();
                return false; 
            }
        }
        now++;
    }
    if (model.empty()) return false;
    else return true;
}

void Preprocessor::judgeSize() {
    // cout << _literals.size() << " " << _coefficients.size() << " " << _upperBound.size() << " " << _lowerBound.size() << " " << _constraintCnt << std::endl;
    if (_coefficients.size() != _constraintCnt ||
        _options.size() != _constraintCnt ||
        _upperBound.size() != _constraintCnt ||
        _lowerBound.size() != _constraintCnt) showError("Wrong size in preprocessor");
}

/* Not implement yet
void Preprocessor::updateFormula(const vector<Int>& nLiteral, const vector<Int>& nCoef, const Int& lb, const Int & ub) {
    if (nLiteral.size() == 0) return;
    // cout << "updateFormula: "; printConstraint(nLiteral, nCoef, lb, ub); 
    bool haveEqConstraint = false;
    Int coefSum = getCoefSum(nCoef);
    for (Int i = 0; i < _literals.size(); i++) {
        ConstraintRelation relation = judgeConstraints(nLiteral, nCoef, _literals[i], _coefficients[i]);
        if (relation == ConstraintRelation::EQUAL) {
            _upperBound[i] = std::min(_upperBound[i], ub);
            _lowerBound[i] = std::max(_lowerBound[i], lb);
            // reverseConstraint(i);
            haveEqConstraint = true;
        } else if (relation == ConstraintRelation::_EQUAL) {
            _upperBound[i] = std::min(_upperBound[i], coefSum - lb);
            _lowerBound[i] = std::max(_lowerBound[i], coefSum - ub);
            haveEqConstraint = true;
        }
    }
    if (!haveEqConstraint) {
        // printConstraint(nLiteral, nCoef, lb, ub);
        addConstraint(nLiteral, nCoef, lb, ub);
    }
    // cout << "updateFormula done" << std::endl;
}
*/

void Preprocessor::addMonomial(Int consIndex, Int lit, Int coef) {
    assert(_literals[consIndex].count(lit) == 0 && _literals[consIndex].count(-lit) == 0);
    _literals[consIndex].insert(lit);
    _coefficients[consIndex].insert({lit, coef});
    _curCoefSum[consIndex] += coef;
}

void Preprocessor::addConstraint(const vector<Int>& nLiteral, const vector<Int>& nCoef, const Int& lb, const Int& ub) {
    if (lb == 0 && ub >= getCoefSum(nCoef)) return;        // always true
    _literals.push_back(Set<Int>());
    _coefficients.push_back(Map<Int, Int>());
    _upperBound.push_back(ub);
    _lowerBound.push_back(lb);

    _curCoefSum.push_back(0);
    _curCoefVal.push_back(0);
    _delLiterals.push_back(Set<Int>());

    for (Int i = 0, sz = nLiteral.size(); i < sz; i++) {
       addMonomial(_constraintCnt, nLiteral[i], nCoef[i]); 
    }

    if (lb == ub) _options.push_back(EQUAL_WORD);
    else _options.push_back(LEQUAL_WORD);

    _constraintCnt++; 

    if (true) judgeSize();
}

void Preprocessor::propagateEqConstraint(Int i) {
    if (_literals[i].size() == 1) {
        propagateUnitEqConstraint(i);
    } 
    else if (_literals[i].size() == 2) {
        propagateSize2EqConstraint(i);
    }
}

void Preprocessor::propagateUnitEqConstraint(Int i) {
    if (_literals[i].size() != 1) showWarning("Unit Propagation with more than one literals");
    if (_upperBound[i] != _lowerBound[i]) showError("Wrong unit Equal Constraint");
    // if (_coefficients[i][0] != _upperBound[i] && _upperBound[i] != 0) showError("UNSAT unit Equal Constraint");

    Int lit;
    vector<Int> model;
    if (getOneModel(i, model)) {
        lit = model[0];
    } else showError("Unit Propagation UNSAT");

    // cout << "Unit Propagation : " << lit << std::endl;

    for (Int j = 0; j < _constraintCnt; j++) {
        if (i == j) continue;

        if (_literals[j].count(lit) > 0) {           // lit == true
            // Int coef = _coefficients[j][id];
            // Int coefSum = getCoefSum(_coefficients[j]) - coef;

            Int coef = _coefficients[j][lit];
            _curCoefSum[j] -= coef;

            _literals[j].erase(lit);
            _coefficients[j].erase(lit);

            _upperBound[j] = std::min(_upperBound[j] - coef, _curCoefSum[j]);
            _lowerBound[j] = std::max(_lowerBound[j] - coef, Int(0));
        }
        else if (_literals[j].count(-lit) > 0) {   // lit in cons is fasle
            Int coef = _coefficients[j][-lit];
            _curCoefSum[j] -= coef;

            _literals[j].erase(-lit);
            _coefficients[j].erase(-lit);
        }
        // updateFormula(_literals[i], _coefficients[i], _lowerBound[i], _upperBound[i]);
    }
}

void Preprocessor::propagateSize2EqConstraint(Int i) {  // x_1 + x_2 = 1 
    // Not implemented yet
    return ;

/*
    if (_literals[i].size() != 2) showError("More than 2 literals while propagation");
    else if (_coefficients[i].at(0) != _coefficients[i].at(1) || 
            _coefficients[i].at(0) != _upperBound[i] || 
            _upperBound[i] != _lowerBound[i]) showError("Wrong propagation size 2 format");
    
    const Int lit1 = _literals[i].at(0);
    const Int lit2 = _literals[i].at(1);  

    // cout << "replace " << lit2 << " by " << -lit1 << std::endl;
    
    //    replace x2  -> (1 - x1) -> x-1
    // or replace x-2 -> (1 - x2) -> x1
    
    for (Int j = 0; j < _literals.size(); j++) {
        if (i == j) continue;
        Int id1 = findVar(_literals[i], lit1);
        Int id2 = findVar(_literals[i], lit2);


        if (id2 == -1) continue;
        else if (id1 == -1) {
            Int tmpCoef = _coefficients[i].at(id2);
            Int tmpLit  = lit1;
            // Int coefSum = getCoefSum(_coefficients[i]);

            //    replace x2  -> (1 - x1) -> x-1
            // or replace x-2 -> (1 - x2) -> x1

            if (lit2 == _literals[i].at(id2)) tmpLit = -tmpLit;
            // else if (lit2 == -literals[i].at(id2))
            //     _lowerBound[i] += tmpCoef, _upperBound[i] += tmpCoef;
            
            _literals[i].erase(_literals[i].begin() + id2);
            _coefficients[i].erase(_coefficients[i].begin() + id2);

            for (Int p = 0; p < _literals[i].size(); p++) {
                if (util::getPbfVar(_literals[i].at(p)) > util::getPbfVar(lit1)) {
                    id1 = p;
                    _literals[i].insert(_literals[i].begin() + id1, tmpLit);
                    _coefficients[i].insert(_coefficients[i].begin() + id1, tmpCoef);
                    break;
                }
            }
            if (id1 == -1) {
                id1 = _literals[i].size();
                _literals[i].push_back(tmpLit);
                _coefficients[i].push_back(tmpCoef);
            }
        } else {

            //    replace x2  -> (1 - x1) -> x-1
            // or replace x-2 -> (1 - x2) -> x1
            Int tmpCoef = _coefficients[i].at(id2);
            Int tmpLit  = _literals[i].at(id1);
            if (lit2 == -_literals[i].at(id2)) tmpCoef = -tmpCoef;

            _literals[i].erase(_literals[i].begin() + id2);
            _coefficients[i].erase(_coefficients[i].begin() + id2);
            // _lowerBound[i] -= tmpCoef;
            // _upperBound[i] -= tmpCoef;

            if (lit1 == -_literals[i].at(id1)) {
                _coefficients[i].at(id1) += tmpCoef;
            } else {
                _coefficients[i].at(id1) -= tmpCoef;
                
                if (_coefficients[i].at(id1) == 0) {
                    _literals[i].erase(_literals[i].begin() + id1);
                    _coefficients[i].erase(_coefficients[i].begin() + id1);    
                } else if (_coefficients[i].at(id1) <= 0) {
                    _literals[i].at(id1)     = -_literals[i].at(id1);
                    _coefficients[i].at(id1) = -_coefficients[i].at(id1);
                    _upperBound[i] += _coefficients[i].at(id1);
                    _lowerBound[i] += _coefficients[i].at(id1);
                }
            }
        }

        if (_literals.size() == 0) {

        }
        
        Int coefSum = getCoefSum(_coefficients[i]);
        _upperBound[i] = std::min(_upperBound[i], coefSum);
        _lowerBound[i] = std::max(_lowerBound[i], Int(0));
        updateFormula(_literals[i], _coefficients[i], _lowerBound[i], _upperBound[i]);
        // reverseConstraint(j);
    }
*/
}

Dd Preprocessor::constructDd(const vector<Pair<Int, Pair<Int, Int> > >& clausePbfVarOrder,
                         vector<Mymap<Pair<Int, Int>, Int, HashFunc, EqualKey> >& itervalToDdIndex,
                         vector<Dd>& structedDd,
                         const Int& index,
                         Int& lf,
                         Int& rt,
                         const Int& limit) const {
    for (auto p : itervalToDdIndex[index]) {
        if (p.first.first <= limit && limit <= p.first.second) {
            lf = p.first.first;
            // rt = p.first.first;
            rt = p.first.second;            // fix a bug?
            return structedDd[p.second];
        }
    }
    Int literal = clausePbfVarOrder[index].second.first;
    Int cnfVar = util::getCnfVar(literal);
    Int varCoef = clausePbfVarOrder[index].second.second;
    Int ddVar = clausePbfVarOrder[index].first;
    Int tlf, trt, flf, frt;
    Dd Bf = constructDd(clausePbfVarOrder, itervalToDdIndex, structedDd, index + 1, flf, frt, limit);
    Dd Bt = constructDd(clausePbfVarOrder, itervalToDdIndex, structedDd, index + 1, tlf, trt, limit - varCoef);
    Dd B  = Dd::getOneDd(mgr);

    if (tlf == flf && trt == frt) {
        B = Bt, lf = tlf + varCoef, rt = trt;
    } else {
        if (util::isPositiveLiteral(literal)) {
            B = Dd::getVarDd(ddVar, mgr).getIte(Bt, Bf);
        } else {
            B = Dd::getVarDd(ddVar, mgr).getIte(Bf, Bt);
        }
        lf = std::max(flf, tlf + varCoef);
        rt = std::min(frt, trt + varCoef);
    }
    structedDd.push_back(B);
    itervalToDdIndex[index][{lf, rt}] = structedDd.size() - 1;
    return B;
}

Dd Preprocessor::constructDdEq(const vector<Pair<Int, Pair<Int, Int> > >& clausePbfVarOrder,
                         vector<Map<Int, Int> >& valueToDdIndex,
                         vector<Dd>& structedDd,
                         const Int& sufCoefSum,
                         const Int& index,
                         const Int& limit) const {

    if (valueToDdIndex[index].count(limit)) {
        return structedDd[valueToDdIndex[index][limit]];
    } else if (limit < 0) {             // UNSAT
        return Dd::getZeroDd(mgr);
    } else if (limit > sufCoefSum) {    // UNSAT
        return Dd::getZeroDd(mgr);
    }

    Int literal = clausePbfVarOrder[index].second.first;
    Int cnfVar = util::getCnfVar(literal);
    Int varCoef = clausePbfVarOrder[index].second.second;
    Int ddVar = clausePbfVarOrder[index].first;
    
    Dd Bf = constructDdEq(clausePbfVarOrder, valueToDdIndex, structedDd, 
                            sufCoefSum - varCoef, index + 1, limit);
    Dd Bt = constructDdEq(clausePbfVarOrder, valueToDdIndex, structedDd, 
                            sufCoefSum - varCoef, index + 1, limit - varCoef);
    Dd B  = Dd::getOneDd(mgr);


    if (util::isPositiveLiteral(literal)) {
        B = Dd::getVarDd(ddVar, mgr).getIte(Bt, Bf);
    } else {
        B = Dd::getVarDd(ddVar, mgr).getIte(Bf, Bt);
    }
        
    structedDd.push_back(B);
    valueToDdIndex[index][limit] = structedDd.size() - 1;
    return B;
}


//  这里需要对出现的 var 进行 cnf 序的重新排列，然后按照 order 来构建 BDD
Dd Preprocessor::getConstraintDd(const Set<Int>& clause, const Map<Int, Int>& coefficient, const string& option, const Int& limit) const {
    Int clauseVarSize = clause.size();
    vector<Pair<Int, Pair<Int, Int> > > clausePbfVarOrder;
    for (const Int& lit : clause) {
        Int var = util::getCnfVar(lit);
        Int coef = coefficient.at(lit);
        clausePbfVarOrder.push_back({var, {lit, coef}});
    }

    std::sort(clausePbfVarOrder.begin(), clausePbfVarOrder.end());  // <ddVar, cnfVar>

    if(option == LEQUAL_WORD) {
        vector<Mymap<Pair<Int, Int>, Int, HashFunc, EqualKey> > itervalToDdIndex(clauseVarSize + 1);
        vector<Dd> structedDd;

        structedDd.push_back(Dd::getZeroDd(mgr));  // false
        structedDd.push_back(Dd::getOneDd(mgr));   // true

        Int coefficientSum = 0;
        for (int i = clauseVarSize - 1; i >= 0; i--) {
            coefficientSum += clausePbfVarOrder[i].second.second;
            itervalToDdIndex[i][{DUMMY_MIN_INT, -1}] = 0;
            itervalToDdIndex[i][{coefficientSum, DUMMY_MAX_INT}] = 1;
        }
        itervalToDdIndex[clauseVarSize][{DUMMY_MIN_INT, -1}] = 0;
        itervalToDdIndex[clauseVarSize][{0, DUMMY_MAX_INT}] = 1;
        Int lf, rt;

        return constructDd(clausePbfVarOrder, itervalToDdIndex, structedDd, 0, lf, rt, limit);
    } else if (option == EQUAL_WORD) {
        vector<Map<Int, Int> > valueToDdIndex(clauseVarSize + 1);
        vector<Dd> structedDd;
        structedDd.push_back(Dd::getZeroDd(mgr));  // false
        structedDd.push_back(Dd::getOneDd(mgr));   // true
        Int coefficientSum = 0;
        for (int i = clauseVarSize - 1; i >= 0; i--) {
            coefficientSum += clausePbfVarOrder[i].second.second;
        }
        valueToDdIndex[clauseVarSize][0] = 1; // present 0 = 0 is always true.
        return constructDdEq(clausePbfVarOrder, valueToDdIndex, structedDd, coefficientSum, 0, limit); 
    } else {
        showError("Undefined option when getConstraintDd");
        return Dd::getOneDd(mgr);
    }
}

// pbf is formated as ai * xi <= K, where ai and K >0
Preprocessor::Preprocessor(const Pbf& pbf) {
    const vector<vector<Int> >& clauses = pbf.getClauses();
    const vector<vector<Int> >& coefficients = pbf.getCoefficients();

    this->_options = pbf.getOptions();
    this->_upperBound = pbf.getLimits();
    this->_apparentVarCnt = pbf.getApparentVarCount();

    _constraintCnt = clauses.size();

    _literals.resize(_constraintCnt);
    _coefficients.resize(_constraintCnt);
    _lowerBound.resize(_constraintCnt);
    _curCoefSum.resize(_constraintCnt);
    _curCoefVal.resize(_constraintCnt);
    _delLiterals.resize(_constraintCnt);

    // init _literals & _coefficients
    for (Int consIndex = 0; consIndex < _constraintCnt ; consIndex++) {
        if (_options[consIndex] == EQUAL_WORD) _lowerBound[consIndex] = _upperBound[consIndex];
        else if (_options[consIndex] == LEQUAL_WORD) _lowerBound[consIndex] = 0;
        else showWarning("Wrong option in preprocessor");

        _curCoefSum[consIndex] = 0;     // sum of unassigned literal's coef
        _curCoefVal[consIndex] = 0;     // sum of assigned literal's coef
        
        for (Int litIndex = 0, vecSize = clauses[consIndex].size(); litIndex < vecSize; litIndex++) {
            addMonomial(consIndex, clauses[consIndex][litIndex], coefficients[consIndex][litIndex]);
        }
    }

    judgeSize();

    // sortConstraints();
}

void Preprocessor::output() {
    cout << "* #variable= " << _apparentVarCnt << " #constraint= " << _constraintCnt << std::endl;
    cout << "*****************" << std::endl;

    for (Int consIndex = 0; consIndex < _constraintCnt; consIndex++) {
        for (const Int& lit : _literals[consIndex]) {
            cout << "+" << _coefficients[consIndex][lit] << " x" << lit << " ";
        }
        cout << _options[consIndex] << " " << _upperBound[consIndex] << " ;" << std::endl;

        if (_options[consIndex] == EQUAL_WORD || _lowerBound[consIndex] == 0 || _lowerBound[consIndex] == _upperBound[consIndex]) continue;

        for (const Int& lit : _literals[consIndex]) {
            cout << "+" << _coefficients[consIndex][lit] << " x" << lit << " ";
        }
        cout << GEQUAL_WORD << " " << _lowerBound[consIndex] << " ;" << std::endl;
    }
}

// do not erase var during propagation
bool Preprocessor::propagateLit(Int lit, Int skipConsIndex) {
    // cout << "Propagate Lit: " << lit << endl;
    for (Int consIndex = 0; consIndex < _constraintCnt; consIndex++) {
        if (consIndex == skipConsIndex) continue;
        assert(_delLiterals[consIndex].count(lit) == 0);

        if (judgeTruthConstraint(consIndex)) continue;

        if (_literals[consIndex].count(lit) > 0) {          // lit == true
            delLitFromCons(consIndex, lit, true);
        }
        else if (_literals[consIndex].count(-lit) > 0) {   // lit in cons is fasle
            delLitFromCons(consIndex, -lit, false);
        } 

        // find conflict
        if (_curCoefVal[consIndex] + 0 > _upperBound[consIndex]) return false;  // all coef > 0
        if (_curCoefVal[consIndex] + _curCoefSum[consIndex] < _lowerBound[consIndex]) return false;

        // find conflict
        if (_delLiterals.size() == _literals[consIndex].size()) {          // deleted all vars  lb <= curCoefVal <= ub
            if (!judgeTruthConstraint(consIndex)) return false;
        }
    }
    // no conflict
    return true;
}

bool Preprocessor::rstPropagateLit(Int lit, Int skipConsIndex) {
    // cout << "Reset Propagate Lit: " << lit << endl;
    for (Int consIndex = 0; consIndex < _constraintCnt; consIndex++) {
        if (consIndex == skipConsIndex) continue;

        if (_delLiterals[consIndex].count(lit) > 0) {          // lit == true
            rstLitFromCons(consIndex, lit, true);
            _delLiterals[consIndex].erase(lit);
        }
        else if (_delLiterals[consIndex].count(-lit) > 0) {   // lit in cons is fasle
            rstLitFromCons(consIndex, -lit, false);
            _delLiterals[consIndex].erase(-lit);
        } 
    }
    return true;
}

/** 
 * @brief judge if cons[consIndex] is implicated by other constraits
 * @return true consIndex can been removed, while false can not
*/
bool Preprocessor::judgeImplication(const Int& consIndex) {
    Dd B = getConstraintDd(_literals[consIndex], _coefficients[consIndex], _options[consIndex], _upperBound[consIndex]);
    // B.writeDotFile(mgr, "./dot/");
    Set <Int> eqCnfLits;

    // return visCnfAssignOnDd(B, eqCnfLits, consIndex);       // version 1 - judge on leaf
    return visAndJudgeAssignOnDd(B, consIndex);             // vsrsion 2 - judge while defs

    // if (visCnfAssignOnDd(B, eqCnfLits, consIndex) != visAndJudgeAssignOnDd(B, consIndex)) {
        // B.writeDotFile(mgr);
        // printFormula();
        // cout << consIndex << " !!!!! "; printConstraint(consIndex);
    // }
    // return visCnfAssignOnDd(B, eqCnfLits, consIndex);       // version 1 - judge on leaf
    // return visAndJudgeAssignOnDd(B, consIndex);             // vsrsion 2 - judge while defs
}

/** 
 * @brief vis all sat assignment on Dd
 * @return true: sat assignments are implicated by other constraints except [consIndex]
 *               i.e. UP(lit : cnfLits) -> \bot for all unsat assignments
 *         false: UP(~ (lit : cnfLits)) not -> \bot
 * 
 * we need all truth assignments find conflicts to remove [consIndex]
*/
bool Preprocessor::visCnfAssignOnDd(const Dd& dd, Set<Int>& cnfLits, const Int& consIndex) {
    if (dd.isLeaf()) {
        // if (dd.leafStr() == "0") return true;
        // else if (dd.leafStr() == "1") return judgeCnfAssign(cnfLits, consIndex);
        if (dd.leafStr() == "0") return judgeCnfAssign(cnfLits, consIndex);
        else if (dd.leafStr() == "1") return true;
        else util::showError("Unexcept Leaf Node with " + dd.leafStr());
    }

    Int var = dd.getTopVar();
    assert(var >= 0);
    
    // go with high edge (then)
    cnfLits.insert(var);
    if (!visCnfAssignOnDd(dd.getThen(), cnfLits, consIndex)) {
        cnfLits.erase(var);
        return false;
    }
    cnfLits.erase(var);

    // go with low edge (else)
    cnfLits.insert(-var);
    if (!visCnfAssignOnDd(dd.getElse(), cnfLits, consIndex)) {
        cnfLits.erase(-var);
        return false;
    }
    cnfLits.erase(-var);

    return true;
}

bool Preprocessor::visAndJudgeAssignOnDd(const Dd& dd, const Int& consIndex) {
    if (dd.isLeaf()) {
        if (dd.leafStr() == "0") return false;          // unsat assignment with no conflict
        else if (dd.leafStr() == "1") return true;      // not consider sat assignmnet
        else util::showError("Unexcept Leaf Node with " + dd.leafStr());
    }

    Int var = dd.getTopVar();
    assert(var >= 0);

    if(!propagateLit(var, consIndex)) {     // have conflict, whatever sat or unsat assignment
        // cout << "Find conflict while propagate: " << var << endl;
        // rstPropagateLit(var, consIndex);
    } else if(!visAndJudgeAssignOnDd(dd.getThen(), consIndex)) {
        rstPropagateLit(var, consIndex);
        return false;                       // false - end dfs
    }
    rstPropagateLit(var, consIndex);

    if(!propagateLit(-var, consIndex)) {    // have conslict
        // cout << "Find conflict while propagate: " << -var << endl;
        // rstPropagateLit(-var, consIndex);
    } else if(!visAndJudgeAssignOnDd(dd.getElse(), consIndex)) {
        rstPropagateLit(-var, consIndex);
        return false;
    }
    rstPropagateLit(-var, consIndex);

    return true;
}

// we need all unsat assignments find conflict
bool Preprocessor::judgeCnfAssign(const Set<Int>& cnfLits, const Int& consIndex) {
    bool conflict = false;

    for (const Int& lit : cnfLits) {
        if (!propagateLit(lit, consIndex)) {
            conflict = true;
            break;
        }
    }

    for (Int i = 0; i < _constraintCnt; i++) {
        for (const Int& lit : _delLiterals[i]) {
            rstLitFromCons(i, lit, (cnfLits.count(lit) > 0));
        }
        _delLiterals[i].clear();
    }
    // cout << "judge Cnf Assign: " << conflict << endl;
    return conflict;
}

void Preprocessor::doVivif(Pbf& pbf) {
    TimePoint vivifStartTime = util::getTimePoint();;
    Int removeCnt = 0;
    for (Int consIndex = 0; consIndex < _constraintCnt; consIndex++) {
        if (_literals[consIndex].size() >= 20) continue;
        if (judgeImplication(consIndex)) {
            // cout << "Remove constraint: "; printConstraint(consIndex);
            removeConstraint(consIndex);
            consIndex--;
            removeCnt++;
        }
    }

    pbf.clearConstraints();
    vector<Int> clause;
    vector<Int> coeff;

    clause.reserve(pbf.getApparentVarCount());
    coeff.reserve(pbf.getApparentVarCount());
    for (Int consIndex = 0; consIndex < _constraintCnt; consIndex++) {
        clause.clear();
        coeff.clear();

        for (const Int& lit : _literals[consIndex]) {
            clause.push_back(lit);
            coeff.push_back(_coefficients[consIndex][lit]);
        }
        
        pbf.addConstraint(clause, coeff, _options[consIndex], _upperBound[consIndex]);
    }

    printComment("Preprocess: vivification delete Constraints num = " + to_string(removeCnt) + " UseTime: " + to_string(util::getSeconds(vivifStartTime)));
}

void Preprocessor::doBackBone(Pbf &pbf) {
    TimePoint backBoneStartTime = util::getTimePoint();
    vector<Int> backBone;
    rsSolver::getBackBone(pbf, backBone);
    printComment("Preprocess: Find Backbone size = " + to_string(backBone.size()) + " UseTime: " + to_string(util::getSeconds(backBoneStartTime)));

    for (Int i = 0; i < backBone.size(); i++) {
        addConstraint({backBone[i]}, {1}, 1, 1);
        propagateEqConstraint(_literals.size() - 1);
    }

    removeTruthConstraints();

    pbf.clearConstraints();
    vector<Int> clause;
    vector<Int> coeff;

    clause.reserve(pbf.getApparentVarCount());
    coeff.reserve(pbf.getApparentVarCount());
    for (Int consIndex = 0; consIndex < _constraintCnt; consIndex++) {
        clause.clear();
        coeff.clear();

        for (const Int& lit : _literals[consIndex]) {
            clause.push_back(lit);
            coeff.push_back(_coefficients[consIndex][lit]);
        }
        
        pbf.addConstraint(clause, coeff, _options[consIndex], _upperBound[consIndex]);
    }
    printComment("Process BackBone Done, UseTime: " + to_string(util::getSeconds(backBoneStartTime)));
}

void Preprocessor::getPreprocessedPbf(Pbf &pbf) {
    // doBackBone(pbf);
    cout << endl;
    printComment("Start Preprocessing ...");

    doBackBone(pbf);
    // init
    doVivif(pbf);
    // pbf.printConstraints();
}

void Preprocessor::test(int argc, char** argv){
    // Pbf pbf(argv[1], WeightFormat::UNWEIGHTED);
    Pbf pbf(argv[1], WeightFormat::WEIGHTED);

    if (ddPackage == SYLVAN_PACKAGE) {  // initializes Sylvan
        cout << "Init sylvan package" << endl;
        lace_init(1ll, 0);
        lace_startup(0, NULL, NULL);
        sylvan::sylvan_set_limits(maxMem * MEGA, tableRatio, initRatio);
        sylvan::sylvan_init_package();
        sylvan::sylvan_init_mtbdd();
        if (multiplePrecision) {
            sylvan::gmp_init();
        }
    }

    // vector<Int> backBone;
    // rsSolver::getBackBone(pbf, backBone);

    Preprocessor preprocessor(pbf);
    preprocessor.doVivif(pbf);

    preprocessor.output();
}