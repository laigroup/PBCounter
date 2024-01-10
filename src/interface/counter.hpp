#pragma once

#include "join.hpp"
#include "pbformula.hpp"
#include "visual.hpp"
#include "preprocess.hpp"
#include "ddNode.hpp"


namespace diagram {
    Float getTerminalValue(const ADD& terminal);
    Float countConstDdFloat(const ADD& dd);
    Int countConstDdInt(const ADD& dd);
    void printMaxDdVarCount(Int maxDdVarCount);
}  // namespace diagram

class Counter {  // abstract
protected:
    // static WeightFormat weightFormat;     // 静态变量, 全局统一
    Cudd mgr;              // 管理 ADD
    VarOrderingHeuristic ddVarOrderingHeuristic;
    bool inverseDdVarOrdering;       // 是否逆序
    Map<Int, Int> cnfVarToDdVarMap;  // e.g. {42: 0, 13: 1}
    vector<Int> ddVarToCnfVarMap;    // e.g. [42, 13], i.e. ddVarOrdering

    JoinNonterminal* joinRoot;

    static void handleSignals(int signal);  // `timeout` sends SIGTERM

    void writeDotFile(Dd& dd, const string& dotFileDir = DOT_DIR);
    template <typename T>
    Set<Int> getCnfVars(const T& ddVars) {
        Set<Int> cnfVars;
        for (Int ddVar : ddVars)
            cnfVars.insert(ddVarToCnfVarMap.at(ddVar));
        return cnfVars;
    }
    const vector<Int>& getDdVarOrdering() const;  // ddVarToCnfVarMap
    void orderDdVars(const Pbf& pbf);             // writes: cnfVarToDdVarMap, ddVarToCnfVarMap
    void printCnfToDdVarMap() const;
    
    Dd getConstraintDd(const vector<Int>& clause, const vector<Int>& coefficient, const string& option, const Int& limit) const;
    Dd constructDd(const vector<Pair<Int, Pair<Int, Int> > >& clausePbfVarOrder,
                    vector<Mymap<Pair<Int, Int>, Int, HashFunc, EqualKey>>& itervalToDdIndex,
                    vector<Dd>& structedDd,
                    const Int& index,
                    Int& lf,
                    Int& rt,
                    const Int& limit) const;
    Dd constructDdEq(const vector<Pair<Int, Pair<Int, Int> > >& clausePbfVarOrder,
                    vector<Map<Int, Int>>& valueToDdIndex,
                    vector<Dd>& structedDd,
                    const Int& sufCoefSum,
                    const Int& index,
                    const Int& limit) const;

    void abstract(Dd& dd, Int ddVar, const Map<Int, Number> &literalWeights);
    void abstractCube(Dd& dd, const Set<Int>& ddVars, const Map<Int, Number> &literalWeights);

    void printJoinTree(const Pbf& pbf) const;

public:
    virtual void constructJoinTree(const Pbf& pbf) = 0;  // handles cnf without empty clause
    void setJoinTree(const Pbf& pbf);                    // handles cnf with/without empty clause

    Dd countSubtree(JoinNode* joinNode, const Pbf& cnf, Set<Int>& projectedCnfVars);  // handles cnf without empty clause
    Float countJoinTree(const Pbf& cnf);                                               // handles cnf with/without empty clause

    virtual Number computeModelCount(const Pbf& pbf) = 0;  // handles cnf without empty clause
    Number getModelCount(const Pbf& pbf);                  // handles cnf with/without empty clause

    void output(const string& filePath, WeightFormat weightFormat);
};

class MonolithicCounter : public Counter {  // builds an ADD for the entire CNF
   protected:
    void setMonolithicClauseDds(vector<Dd>& clauseDds, const Pbf& cnf);
    void setCnfDd(Dd& cnfDd, const Pbf& pbf);

   public:
    void constructJoinTree(const Pbf& pbf) override;
    Number computeModelCount(const Pbf& pbf) override;
    MonolithicCounter(VarOrderingHeuristic ddVarOrderingHeuristic, bool inverseDdVarOrdering);
};

class FactoredCounter : public Counter {};  // abstract; builds an ADD for each clause

class LinearCounter : public FactoredCounter {  // combines adjacent clauses
   protected:
    vector<Set<Int> > projectableCnfVarSets;  // clauseIndex |-> cnfVars

    void fillProjectableCnfVarSets(const vector<vector<Int>>& clauses);
    void setLinearClauseDds(vector<Dd>& clauseDds, const Pbf& pbf);

   public:
    void constructJoinTree(const Pbf& pbf) override;
    Number computeModelCount(const Pbf& pbf) override;
    LinearCounter(
        VarOrderingHeuristic ddVarOrderingHeuristic,
        bool inverseDdVarOrdering);
};

class NonlinearCounter : public FactoredCounter {  // abstract; puts clauses in clusters
   protected:
    bool usingTreeClustering;
    VarOrderingHeuristic cnfVarOrderingHeuristic;
    bool inverseCnfVarOrdering;
    vector<vector<Int>> clusters;  // clusterIndex |-> clauseIndices

    vector<Set<Int>> occurrentCnfVarSets;    // clusterIndex |-> cnfVars
    vector<Set<Int>> projectableCnfVarSets;  // clusterIndex |-> cnfVars
    vector<vector<JoinNode*>> joinNodeSets;  // clusterIndex |-> non-null nodes

    vector<vector<Dd> > ddClusters;        // clusterIndex |-> ADDs (if usingTreeClustering)
    vector<Set<Int>> projectingDdVarSets;  // clusterIndex |-> ddVars (if usingTreeClustering)

    void printClusters(const vector<vector<Int>>& clauses, const vector<vector<Int>>& coefficient, const vector<string> &options, const vector<Int>& limits) const;
    void fillClusters(const vector<vector<Int>>& clauses, const vector<Int>& cnfVarOrdering, bool usingMinVar);

    void printOccurrentCnfVarSets() const;
    void printProjectableCnfVarSets() const;
    void fillCnfVarSets(const vector<vector<Int>>& clauses, bool usingMinVar);  // writes: occurrentCnfVarSets, projectableCnfVarSets

    Set<Int> getProjectingDdVars(Int clusterIndex, bool usingMinVar, const vector<Int>& cnfVarOrdering, const vector<vector<Int>>& clauses);
    void fillDdClusters(const vector<vector<Int>>& clauses, const vector<vector<Int>>& coefficients, const vector<string>& options, const vector<Int>& limits, const vector<Int>& cnfVarOrdering, bool usingMinVar);           // (if usingTreeClustering)
    void fillProjectingDdVarSets(const vector<vector<Int>>& clauses, const vector<vector<Int>>& coefficients, const vector<string>& options, const vector<Int>& limits, const vector<Int>& cnfVarOrdering, bool usingMinVar);  // (if usingTreeClustering)

    Int getTargetClusterIndex(Int clusterIndex) const;                                                                  // returns DUMMY_MAX_INT if no var remains
    Int getNewClusterIndex(const Dd& abstractedClusterDd, const vector<Int>& cnfVarOrdering, bool usingMinVar) const;  // returns DUMMY_MAX_INT if no var remains (if usingTreeClustering)
    Int getNewClusterIndex(const Set<Int>& remainingDdVars) const;                                                      // returns DUMMY_MAX_INT if no var remains (if usingTreeClustering) #MAVC

    void constructJoinTreeUsingListClustering(const Pbf& pbf, bool usingMinVar);
    void constructJoinTreeUsingTreeClustering(const Pbf& pbf, bool usingMinVar);

    Number countUsingListClustering(const Pbf& pbf, bool usingMinVar);
    Number countUsingTreeClustering(const Pbf& pbf, bool usingMinVar);
};

class BucketCounter : public NonlinearCounter {  // bucket elimination
   public:
    void constructJoinTree(const Pbf& pbf) override;
    Number computeModelCount(const Pbf& pbf) override;
    BucketCounter(
        bool usingTreeClustering,
        VarOrderingHeuristic cnfVarOrderingHeuristic,
        bool inverseCnfVarOrdering,
        VarOrderingHeuristic ddVarOrderingHeuristic,
        bool inverseDdVarOrdering);
};

class BouquetCounter : public NonlinearCounter {  // Bouquet's Method
   public:
    void constructJoinTree(const Pbf& pbf) override;
    Number computeModelCount(const Pbf& pbf) override;
    BouquetCounter(
        bool usingTreeClustering,
        VarOrderingHeuristic cnfVarOrderingHeuristic,
        bool inverseCnfVarOrdering,
        VarOrderingHeuristic ddVarOrderingHeuristic,
        bool inverseDdVarOrdering);
};