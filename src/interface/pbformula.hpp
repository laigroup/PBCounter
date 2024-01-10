/* PB Constraint */
#pragma once

/* inclusions *****************************************************************/

#include "graph.hpp"

/* constants ******************************************************************/

extern const string &WEIGHT_WORD;

/* classes ********************************************************************/

class PbLabel : public vector<Int> {
public: 
    void addNumber(Int i);
};

class Pbf {
protected:
    Int declaredVarCount = DUMMY_MAX_INT;
    Int apparentVarCount = DUMMY_MIN_INT;
    WeightFormat weightFormat;
    Map<Int, Number> literalWeights;
    vector<vector<Int> > clauses;
    vector<vector<Int> > coefficients;
    vector<string> options;
    vector<Int> limits;
    vector<Int> apparentVars; // vars appearing in clauses, ordered by 1st appearance
    
    void updateApparentVars(Int literal); // adds var to apparentVars
    Graph getGaifmanGraph() const;
    vector<Int> getAppearanceVarOrdering() const;
    vector<Int> getDeclarationVarOrdering() const;
    vector<Int> getRandomVarOrdering() const;
    vector<Int> getLexpVarOrdering() const;
    vector<Int> getLexmVarOrdering() const;
    vector<Int> getMcsVarOrdering() const;

public:
    void addConstraint(const vector<Int> &clause, const vector<Int> &coefficent, const string& option, const Int &limit); // writes: clauses, apparentVars

    vector<Int> getVarOrdering(VarOrderingHeuristic varOrderingHeuristic, bool inverse) const;
    Int getDeclaredVarCount() const;
    Int getApparentVarCount() const;
    Map<Int, Number> getLiteralWeights() const;
    Int getEmptyClauseIndex() const; // first (nonnegative) index if found else DUMMY_MIN_INT

    const vector<vector<Int>> &getClauses() const;
    const vector<vector<Int>> &getCoefficients() const;
    const vector<string> &getOptions() const;
    const vector<Int> &getLimits() const;
    const vector<Int> &getApparentVars() const;

    // only clear constraint, not weightFormat apparentVars literalWeight 
    void clearConstraints();

    void setClauses(vector<vector<Int> > clauses);
    void setCoefficients(vector<vector<Int> > coef);
    void setOptions(vector<string> options);
    void setLimits(vector<Int> limits);

    void printConstraints() const;
    void sortConstraintsByOrdering();
    Pbf(const string &filePath, WeightFormat weightFormat);
    Pbf(const vector<vector<Int>> &clauses, const vector<vector<Int>> &coefficients, const vector<string> & options, const vector<Int> &limits);
};