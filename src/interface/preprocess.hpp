#pragma once

/* inclusions *****************************************************************/

#include "pbformula.hpp"
#include "rsInterface.hpp"
#include "ddNode.hpp"


enum class ConstraintRelation { EQUAL,
                                GEQUAL,     // A >=  B . A include B
                                LEQUAL,     // A <=  B . B include A
                                _EQUAL,
                                _GEQUAL,    // A >= _B . _B is reverse(B)
                                _LEQUAL,    // A <= _B . _B is reverse(B)
                                NEQUAL      // A != B
                            };

class Preprocessor {
protected:
    // formated as      0 < ai * xi <= K
    vector<Set<Int> >      _literals;
    vector<Map<Int, Int> > _coefficients;
    vector<Int>            _upperBound;
    vector<Int>            _lowerBound;
    vector<string>         _options;

    vector<Int>            _curCoefSum;
    vector<Int>            _curCoefVal;

    vector<Set<Int> >      _delLiterals;       // literal in constraint.at(i) is deleted, only store positive literal (var)
    vector<Int>            _assignVariables;    // variables with 

    Int _apparentVarCnt;
    Int _constraintCnt;

    Cudd mgr;              // 管理 ADD

    Int getCoefSum(Int consIndex) const;
    Int getCoefSum(const vector<Int> &coefficient) const;
    Int findVar(const vector<Int> &literal, const Int &var);

    void removeConstraint(Int id);
    void removeTruthConstraints();

    inline bool judgeTruthConstraint(Int consIndex) const;
    inline bool isDelLitFromCons(Int consIndex, Int lit) const { return _delLiterals[consIndex].count(lit) > 0;}

    void delLitFromCons(Int consIndex, Int lit, bool trueAssign);    // delete var from consIndex
    void rstLitFromCons(Int consIndex, Int lit, bool trueAssign);    // restore var in consIndex
   
    void printConstraint(Int consIndex) const;
    void printFormula() const;

    ConstraintRelation judgeConstraints(const Set<Int>& literal, const Map<Int, Int>& coefficient, 
                                        const Set<Int>& _literal, const Map<Int, Int>& _coefficient) const;
    
    bool getOneModel(Int id, vector<Int> &model) const;
    
    void judgeSize();
    // void updateFormula(const vector<Int>& nLiteral, const vector<Int>& nCoef, const Int& lb, const Int& ub);
    void addMonomial(Int consIndex, Int lit, Int coef);
    void addConstraint(const vector<Int>& nLiteral, const vector<Int>& nCoef, const Int& lb, const Int& ub);
    
    void propagateEqConstraint(Int id);
    void propagateUnitEqConstraint(Int id);
    void propagateSize2EqConstraint(Int id);

    bool propagateLit(Int lit, Int skipConsIndex = -1);
    bool rstPropagateLit(Int lit, Int skipConsIdex = -1);

    bool visCnfAssignOnDd(const Dd& dd, Set<Int>& cnfLits, const Int& consIndex);
    bool judgeCnfAssign(const Set<Int>& cnfLits, const Int& consIndex);
    bool judgeImplication(const Int& consIndex);

    bool visAndJudgeAssignOnDd(const Dd& dd, const Int& consIndex);

    Dd getConstraintDd(const Set<Int>& clause, const Map<Int, Int>& coefficient, const string& option, const Int& limit) const;
    Dd constructDd(const vector<Pair<Int, Pair<Int, Int> > >& clausePbfVarOrder,
                    vector<Mymap<Pair<Int, Int>, Int, HashFunc, EqualKey> >& itervalToDdIndex,
                    vector<Dd>& structedDd,
                    const Int& index,
                    Int& lf,
                    Int& rt,
                    const Int& limit) const;
    Dd constructDdEq(const vector<Pair<Int, Pair<Int, Int> > >& clausePbfVarOrder,
                    vector<Map<Int, Int> >& valueToDdIndex,
                    vector<Dd>& structedDd,
                    const Int& sufCoefSum,
                    const Int& index,
                    const Int& limit) const;

    void output();

public:
    Preprocessor(const Pbf&);

    void doVivif(Pbf &pbf);
    void doBackBone(Pbf &pbf);
    void getPreprocessedPbf(Pbf &pbf);


    static void test(int, char**);
};