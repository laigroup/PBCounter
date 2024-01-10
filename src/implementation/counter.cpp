#include "../interface/counter.hpp"

Float diagram::getTerminalValue(const ADD& terminal) {
    DdNode* node = terminal.getNode();
    return (node->type).value;
}

Float diagram::countConstDdFloat(const ADD& dd) {
    ADD minTerminal = dd.FindMin();
    ADD maxTerminal = dd.FindMax();

    Float minValue = getTerminalValue(minTerminal);
    Float maxValue = getTerminalValue(maxTerminal);

    if (minValue != maxValue) {
        showError("ADD is nonconst: min value " + to_string(minValue) +
                  ", max value " + to_string(maxValue));
    }

    return minValue;
}

Int diagram::countConstDdInt(const ADD& dd) {
    Float value = countConstDdFloat(dd);

    if (!util::isInt(value))
        showError("unweighted model count is not int");

    return value;
}

void diagram::printMaxDdVarCount(Int maxDdVarCount) {
    util::printRow("maxAddVarCount", maxDdVarCount);
}

/* classes ********************************************************************/
/* class Counter **************************************************************/

void Counter::handleSignals(int signal) {
    cout << "\n";
    util::printDuration(startTime);
    cout << "\n";

    util::printSolutionLine(Number(0), WeightFormat::UNWEIGHTED, 0, 0);
    showError("received system signal " + to_string(signal) + "; printed dummy model count");
}

void Counter::writeDotFile(Dd& dd, const string& dotFileDir) {
    dd.writeDotFile(mgr, dotFileDir);
}

const vector<Int>& Counter::getDdVarOrdering() const {
    return ddVarToCnfVarMap;
}

void Counter::orderDdVars(const Pbf& pbf) {
    ddVarToCnfVarMap = pbf.getVarOrdering(ddVarOrderingHeuristic, inverseDdVarOrdering);
    for (Int ddVar = 0; ddVar < ddVarToCnfVarMap.size(); ddVar++) {
        Int cnfVar = ddVarToCnfVarMap[ddVar];
        cnfVarToDdVarMap[cnfVar] = ddVar;
        // 这里是 ADD 的变量名，用于统一，之后通过 Map 映射和 cnf 变量对应
        mgr.addVar(ddVar);  // creates ddVar-th ADD var
    }
    if (verbosityLevel >= 2) {
        printCnfToDdVarMap();
    }
}

void Counter::printCnfToDdVarMap() const {
    Int varSize = ddVarToCnfVarMap.size();
    util::printComment("print CnfVar to DdVar map");
    for (Int i = 0; i < varSize; i++) {
        util::printComment("x" + to_string(ddVarToCnfVarMap[i]) + " -> " + to_string(i));
    }
}

Dd Counter::constructDd(const vector<Pair<Int, Pair<Int, Int> > >& clausePbfVarOrder,
                         vector<Mymap<Pair<Int, Int>, Int, HashFunc, EqualKey>>& itervalToDdIndex,
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

Dd Counter::constructDdEq(const vector<Pair<Int, Pair<Int, Int> > >& clausePbfVarOrder,
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
Dd Counter::getConstraintDd(const vector<Int>& clause, const vector<Int>& coefficient, const string& option, const Int& limit) const {
    Int clauseVarSize = clause.size();
    vector<Pair<Int, Pair<Int, Int> > > clausePbfVarOrder;
    for (Int i = 0; i < clause.size(); i++) {
        Int literal = clause[i];
        Int var = util::getCnfVar(literal);
        Int coef = coefficient[i];
        clausePbfVarOrder.push_back({cnfVarToDdVarMap.at(var), {literal, coef}});
    }

    std::sort(clausePbfVarOrder.begin(), clausePbfVarOrder.end());  // <ddVar, cnfVar>

    if(option == LEQUAL_WORD) {
        vector<Mymap<Pair<Int, Int>, Int, HashFunc, EqualKey> > itervalToDdIndex(clauseVarSize + 1);
        vector<Dd> structedDd;

        structedDd.push_back(Dd::getZeroDd(mgr));  // false
        structedDd.push_back(Dd::getOneDd(mgr));   // true

        Int coefficientSum = 0;
        for (int i = clauseVarSize - 1; i >= 0; i--) {
            // coefficientSum += clausePbfVarOrder[i].second.second;
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
            // coefficientSum += clausePbfVarOrder[i].second.second;
            coefficientSum += clausePbfVarOrder[i].second.second;
        }
        valueToDdIndex[clauseVarSize][0] = 1; // present 0 = 0 is always true.
        return constructDdEq(clausePbfVarOrder, valueToDdIndex, structedDd, coefficientSum, 0, limit); 
    } else {
        showError("Undefined option when getConstraintDd");
        return Dd::getOneDd(mgr);
    }
}

// Compose 其实就是一个 project 变量, 通过赋值 Dd 的时候计算的过程
// 这里改了加权版本的
void Counter::abstract(Dd& dd, Int ddVar, const Map<Int, Number> &literalWeights) {
    if (verbosityLevel >= 2) util::printComment("project " + to_string(ddVar));

    Int pbfVar = ddVarToCnfVarMap[ddVar];

    Dd positiveWeight = Dd::getConstDd(literalWeights.at(pbfVar), mgr);
    Dd negativeWeight = Dd::getConstDd(literalWeights.at(-pbfVar), mgr);

    Dd highTerm = dd.getComposition(ddVar, true, mgr).getProduct(positiveWeight);
    Dd lowTerm  = dd.getComposition(ddVar, false, mgr).getProduct(negativeWeight);
    
    // dd = positiveWeight * dd.Compose(mgr.addOne(), ddVar) + negativeWeight * dd.Compose(mgr.addZero(), ddVar);
    dd = highTerm.getSum(lowTerm);
}

// dd.Compose
void Counter::abstractCube(Dd& dd, const Set<Int>& ddVars, const Map<Int, Number> &literalWeights) {
    for (Int ddVar : ddVars) {
        abstract(dd, ddVar, literalWeights);
    }
}

void Counter::printJoinTree(const Pbf& pbf) const {
    cout << PROBLEM_WORD << " " << JT_WORD << " " << pbf.getDeclaredVarCount() << " " << joinRoot->getTerminalCount() << " " << joinRoot->getNodeCount() << "\n";
    joinRoot->printSubtree();
}

void Counter::setJoinTree(const Pbf& pbf) {
    if (pbf.getClauses().empty()) {  // empty cnf
        // showWarning("cnf is empty"); // different warning for empty clause
        joinRoot = new JoinNonterminal(vector<JoinNode*>());
        return;
    }

    Int i = pbf.getEmptyClauseIndex();
    if (i != DUMMY_MIN_INT) {  // empty clause found
        showWarning("clause " + to_string(i + 1) + " of cnf is empty (1-indexing); generating dummy join tree");
        joinRoot = new JoinNonterminal(vector<JoinNode*>());
    } else {
        constructJoinTree(pbf);
    }
}

Dd Counter::countSubtree(JoinNode* joinNode, const Pbf& pbf, Set<Int>& projectedCnfVars) {
    if (joinNode->isTerminal()) {
        Int index = joinNode->getNodeIndex();
        return getConstraintDd(pbf.getClauses()[index], pbf.getCoefficients()[index], pbf.getOptions()[index], pbf.getLimits()[index]);
    } else {
        // ADD dd = mgr.addOne();
        Dd dd = Dd::getOneDd(mgr);
        for (JoinNode* child : joinNode->getChildren()) {
            dd = dd.getProduct(countSubtree(child, pbf, projectedCnfVars));
        }
        for (Int cnfVar : joinNode->getProjectableCnfVars()) {
            projectedCnfVars.insert(cnfVar);

            Int ddVar = cnfVarToDdVarMap[cnfVar];
            abstract(dd, ddVar, pbf.getLiteralWeights());
        }
        return dd;
    }
}

Number Counter::getModelCount(const Pbf& pbf) {
    Int i = pbf.getEmptyClauseIndex();
    if (i != DUMMY_MIN_INT) {  // empty clause found
        showWarning("clause " + to_string(i + 1) + " of cnf is empty (1-indexing)");
        return 0;
    } else {
        return computeModelCount(pbf);
    }
}

void Counter::output(const string& filePath, WeightFormat weightFormat) {
    Pbf pbf(filePath, weightFormat);

    if(preprocessorFlag) {
        Preprocessor preprocessor(pbf);
        preprocessor.getPreprocessedPbf(pbf);
    }

    // printComment("After preprocess...");
    // util::printRow("VarCount", pbf.getApparentVarCount());
    // util::printRow("ClauseCount", pbf.getClauses().size());

    printComment("Computing output...", 1);

    signal(SIGINT, handleSignals);   // Ctrl c
    signal(SIGTERM, handleSignals);  // timeout

    util::printSolutionLine(getModelCount(pbf), weightFormat);

    if(verbosityLevel >= 2) {
        util::printRow("maxDiagramLeaves", Dd::maxDdLeafCount);
        util::printRow("maxDiagramNodes", Dd::maxDdNodeCount);
    } 
}

/* class MonolithicCounter ****************************************************/

void MonolithicCounter::setMonolithicClauseDds(vector<Dd>& clauseDds, const Pbf& pbf) {
    clauseDds.clear();
    const vector<vector<Int>>& clauses = pbf.getClauses();
    const vector<vector<Int>>& coefficients = pbf.getCoefficients();
    const vector<string>& options = pbf.getOptions();
    const vector<Int>& limits = pbf.getLimits();
    for (int i = 0; i < clauses.size(); i++) {
        Dd clauseDd = getConstraintDd(clauses[i], coefficients[i], options[i], limits[i]);
        clauseDds.push_back(clauseDd);
    }
}

void MonolithicCounter::setCnfDd(Dd& pbfDd, const Pbf& pbf) {
    vector<Dd> clauseDds;
    setMonolithicClauseDds(clauseDds, pbf);
    pbfDd = mgr.addOne();
    for (const Dd& clauseDd : clauseDds) {
        // pbfDd &= clauseDd;  // operator& is operator* in class ADD
        pbfDd = pbfDd.getProduct(clauseDd);
    }
}

void MonolithicCounter::constructJoinTree(const Pbf& pbf) {
    vector<JoinNode*> terminals;
    for (Int clauseIndex = 0; clauseIndex < pbf.getClauses().size(); clauseIndex++) {
        terminals.push_back(new JoinTerminal());
    }

    vector<Int> projectableCnfVars = pbf.getApparentVars();

    joinRoot = new JoinNonterminal(terminals, Set<Int>(projectableCnfVars.begin(), projectableCnfVars.end()));
}

Number MonolithicCounter::computeModelCount(const Pbf& pbf) {
    orderDdVars(pbf);

    Dd cnfDd = Dd::getOneDd(mgr);
    setCnfDd(cnfDd, pbf);

    // Set<Int> support = util::getSupport(cnfDd);
    Set<Int> support = cnfDd.getSupport();
    for (Int ddVar : support) {
        abstract(cnfDd, ddVar, pbf.getLiteralWeights());
    }

    // Float modelCount = diagram::countConstDdFloat(cnfDd);
    Number modelCount = cnfDd.extractConst();
    modelCount = util::adjustModelCount(modelCount, getCnfVars(support), pbf.getLiteralWeights());
    return modelCount;
}

MonolithicCounter::MonolithicCounter(VarOrderingHeuristic ddVarOrderingHeuristic, bool inverseDdVarOrdering) {
    this->ddVarOrderingHeuristic = ddVarOrderingHeuristic;
    this->inverseDdVarOrdering = inverseDdVarOrdering;
}

/* class FactoredCounter ******************************************************/

/* class LinearCounter ******************************************************/

void LinearCounter::fillProjectableCnfVarSets(const vector<vector<Int>>& clauses) {
    projectableCnfVarSets = vector<Set<Int>>(clauses.size(), Set<Int>());

    Set<Int> placedCnfVars;  // cumulates vars placed in projectableCnfVarSets so far
    for (Int clauseIndex = clauses.size() - 1; clauseIndex >= 0; clauseIndex--) {
        Set<Int> clauseCnfVars = util::getClauseCnfVars(clauses[clauseIndex]);

        Set<Int> placingCnfVars;
        util::differ(placingCnfVars, clauseCnfVars, placedCnfVars);
        projectableCnfVarSets[clauseIndex] = placingCnfVars;
        util::unionize(placedCnfVars, placingCnfVars);
    }
}

void LinearCounter::setLinearClauseDds(vector<Dd>& clauseDds, const Pbf& pbf) {
    clauseDds.clear();
    clauseDds.push_back(mgr.addOne());
    // for (const vector<Int> &clause : cnf.getClauses()) {
    // ADD clauseDd = getClauseDd(clause);
    const vector<vector<Int>>& clauses = pbf.getClauses();
    const vector<vector<Int>>& coefficients = pbf.getCoefficients();
    const vector<string> options = pbf.getOptions();
    const vector<Int>& limits = pbf.getLimits();
    for (int i = 0; i < clauses.size(); i++) {
        Dd clauseDd = getConstraintDd(clauses[i], coefficients[i], options[i], limits[i]);
        clauseDds.push_back(clauseDd);
    }
}

void LinearCounter::constructJoinTree(const Pbf& pbf) {
    const vector<vector<Int>>& clauses = pbf.getClauses();
    fillProjectableCnfVarSets(clauses);

    vector<JoinNode*> clauseNodes;
    for (Int clauseIndex = 0; clauseIndex < clauses.size(); clauseIndex++) {
        clauseNodes.push_back(new JoinTerminal());
    }

    joinRoot = new JoinNonterminal({clauseNodes[0]}, projectableCnfVarSets[0]);

    for (Int clauseIndex = 1; clauseIndex < clauses.size(); clauseIndex++) {
        joinRoot = new JoinNonterminal({joinRoot, clauseNodes[clauseIndex]}, projectableCnfVarSets[clauseIndex]);
    }
}

Number LinearCounter::computeModelCount(const Pbf& pbf) {
    orderDdVars(pbf);

    vector<Dd> factorDds;
    setLinearClauseDds(factorDds, pbf);
    Set<Int> projectedCnfVars;
    while (factorDds.size() > 1) {
        Dd factor1 = Dd::getOneDd(mgr), factor2 = Dd::getOneDd(mgr);
        util::popBack(factor1, factorDds);
        util::popBack(factor2, factorDds);

        // ADD product = factor1 * factor2;
        Dd product = factor1.getProduct(factor2);
        Set<Int> productDdVars = product.getSupport();

        Set<Int> otherDdVars = util::getSupportSuperset(factorDds);

        Set<Int> projectingDdVars;
        util::differ(projectingDdVars, productDdVars, otherDdVars);
        abstractCube(product, projectingDdVars, pbf.getLiteralWeights());
        util::unionize(projectedCnfVars, getCnfVars(projectingDdVars));

        factorDds.push_back(product);
    }

    // Number modelCount = diagram::countConstDdFloat(util::getSoleMember(factorDds));
    Number modelCount = util::getSoleMember(factorDds).extractConst();
    modelCount = util::adjustModelCount(modelCount, projectedCnfVars, pbf.getLiteralWeights());
    return modelCount;
}

LinearCounter::LinearCounter(VarOrderingHeuristic ddVarOrderingHeuristic, bool inverseDdVarOrdering) {
    this->ddVarOrderingHeuristic = ddVarOrderingHeuristic;
    this->inverseDdVarOrdering = inverseDdVarOrdering;
}

/* class NonlinearCounter ********************************************************/

void NonlinearCounter::printClusters(const vector<vector<Int>>& clauses, const vector<vector<Int>>& coefficients, const vector<string> &options, const vector<Int>& limits) const {
    printThinLine();
    printComment("clusters {");
    for (Int clusterIndex = 0; clusterIndex < clusters.size(); clusterIndex++) {
        printComment(
            "\t"
            "cluster " +
            to_string(clusterIndex + 1) + ":");
        for (Int clauseIndex : clusters[clusterIndex]) {
            cout << COMMENT_WORD << "\t\t"
                                    "constraint "
                 << clauseIndex + 1 << +":\t";
            util::printConstraint(clauses[clauseIndex], coefficients[clauseIndex], options[clauseIndex], limits[clauseIndex]);
        }
    }
    printComment("}");
    printThinLine();
}

void NonlinearCounter::fillClusters(const vector<vector<Int>>& clauses, const vector<Int>& cnfVarOrdering, bool usingMinVar) {
    clusters = vector<vector<Int>>(cnfVarOrdering.size(), vector<Int>());
    for (Int clauseIndex = 0; clauseIndex < clauses.size(); clauseIndex++) {
        Int clusterIndex = usingMinVar ? util::getMinClauseRank(clauses[clauseIndex], cnfVarOrdering) : util::getMaxClauseRank(clauses[clauseIndex], cnfVarOrdering);
        clusters[clusterIndex].push_back(clauseIndex);
    }
}

void NonlinearCounter::printOccurrentCnfVarSets() const {
    printThinLine();
    printComment("occurrentCnfVarSets {");
    for (Int clusterIndex = 0; clusterIndex < clusters.size(); clusterIndex++) {
        const Set<Int>& cnfVarSet = occurrentCnfVarSets[clusterIndex];
        cout << COMMENT_WORD << "\t"
             << "cluster " << clusterIndex + 1 << ":";
        for (Int cnfVar : cnfVarSet) {
            cout << " " << cnfVar;
        }
        cout << "\n";
    }
    printComment("}");
    printThinLine();
}

void NonlinearCounter::printProjectableCnfVarSets() const {
    printThinLine();
    printComment("projectableCnfVarSets {");
    for (Int clusterIndex = 0; clusterIndex < clusters.size(); clusterIndex++) {
        const Set<Int>& cnfVarSet = projectableCnfVarSets[clusterIndex];
        cout << COMMENT_WORD << "\t"
             << "cluster " << clusterIndex + 1 << ":";
        for (Int cnfVar : cnfVarSet) {
            cout << " " << cnfVar;
        }
        cout << "\n";
    }
    cout << "}\n";
    printComment("}");
    printThinLine();
}

void NonlinearCounter::fillCnfVarSets(const vector<vector<Int>>& clauses, bool usingMinVar) {
    occurrentCnfVarSets = vector<Set<Int>>(clusters.size(), Set<Int>());
    projectableCnfVarSets = vector<Set<Int>>(clusters.size(), Set<Int>());

    Set<Int> placedCnfVars;  // cumulates vars placed in projectableCnfVarSets so far
    for (Int clusterIndex = clusters.size() - 1; clusterIndex >= 0; clusterIndex--) {
        Set<Int> clusterCnfVars = util::getClusterCnfVars(clusters[clusterIndex], clauses);

        occurrentCnfVarSets[clusterIndex] = clusterCnfVars;

        Set<Int> placingCnfVars;
        util::differ(placingCnfVars, clusterCnfVars, placedCnfVars);
        projectableCnfVarSets[clusterIndex] = placingCnfVars;
        util::unionize(placedCnfVars, placingCnfVars);
    }
}

Set<Int> NonlinearCounter::getProjectingDdVars(Int clusterIndex, bool usingMinVar, const vector<Int>& cnfVarOrdering, const vector<vector<Int>>& clauses) {
    Set<Int> projectableCnfVars;

    if (usingMinVar) {  // bucket elimination
        projectableCnfVars.insert(cnfVarOrdering[clusterIndex]);
    } else {  // Bouquet's Method
        Set<Int> activeCnfVars = util::getClusterCnfVars(clusters[clusterIndex], clauses);

        Set<Int> otherCnfVars;
        // 后续的 cluster 还没访问，将后续没有了的 Var 进行 project
        for (Int i = clusterIndex + 1; i < clusters.size(); i++) {
            util::unionize(otherCnfVars, util::getClusterCnfVars(clusters[i], clauses));
        }

        // projectableCnfVars = activeCnfVars \ otherCnfVars
        util::differ(projectableCnfVars, activeCnfVars, otherCnfVars);
    }

    // Mapping
    Set<Int> projectingDdVars;
    for (Int cnfVar : projectableCnfVars) {
        projectingDdVars.insert(cnfVarToDdVarMap[cnfVar]);
    }
    return projectingDdVars;
}

void NonlinearCounter::fillDdClusters(const vector<vector<Int>>& clauses, const vector<vector<Int>>& coefficients, const vector<string>& options, const vector<Int>& limits, const vector<Int>& cnfVarOrdering, bool usingMinVar) {
    fillClusters(clauses, cnfVarOrdering, usingMinVar);
    if (verbosityLevel >= 2)
        printClusters(clauses, coefficients, options, limits);

    ddClusters = vector<vector<Dd> >(clusters.size(), vector<Dd>());
    for (Int clusterIndex = 0; clusterIndex < clusters.size(); clusterIndex++) {
        for (Int clauseIndex : clusters[clusterIndex]) {
            // ADD clauseDd = getClauseDd(clauses[clauseIndex]);
            Dd clauseDd = getConstraintDd(clauses[clauseIndex], coefficients[clauseIndex], options[clauseIndex], limits[clauseIndex]);

            ddClusters[clusterIndex].push_back(clauseDd);
        }
    }
}

void NonlinearCounter::fillProjectingDdVarSets(const vector<vector<Int>>& clauses, const vector<vector<Int>>& coefficients, const vector<string>& options, const vector<Int>& limits, const vector<Int>& cnfVarOrdering, bool usingMinVar) {
    fillDdClusters(clauses, coefficients, options, limits, cnfVarOrdering, usingMinVar);

    projectingDdVarSets = vector<Set<Int>>(clusters.size(), Set<Int>());
    for (Int clusterIndex = 0; clusterIndex < ddClusters.size(); clusterIndex++) {
        projectingDdVarSets[clusterIndex] = getProjectingDdVars(clusterIndex, usingMinVar, cnfVarOrdering, clauses);
    }
}

Int NonlinearCounter::getTargetClusterIndex(Int clusterIndex) const {
    const Set<Int>& remainingCnfVars = occurrentCnfVarSets[clusterIndex];
    for (Int i = clusterIndex + 1; i < clusters.size(); i++) {
        if (!util::isDisjoint(occurrentCnfVarSets[i], remainingCnfVars)) {
            return i;
        }
    }
    return DUMMY_MAX_INT;
}

Int NonlinearCounter::getNewClusterIndex(const Dd& abstractedClusterDd, const vector<Int>& cnfVarOrdering, bool usingMinVar) const {
    if (usingMinVar) {
        return util::getMinDdRank(abstractedClusterDd, ddVarToCnfVarMap, cnfVarOrdering);
    } else {
        // const Set<Int>& remainingDdVars = util::getSupport(abstractedClusterDd);
        const Set<Int>& remainingDdVars = abstractedClusterDd.getSupport();
        for (Int clusterIndex = 0; clusterIndex < clusters.size(); clusterIndex++) {
            if (!util::isDisjoint(projectingDdVarSets[clusterIndex], remainingDdVars)) {
                return clusterIndex;
            }
        }
        return DUMMY_MAX_INT;
    }
}

Int NonlinearCounter::getNewClusterIndex(const Set<Int>& remainingDdVars) const {  // #MAVC
    for (Int clusterIndex = 0; clusterIndex < clusters.size(); clusterIndex++) {
        if (!util::isDisjoint(projectingDdVarSets[clusterIndex], remainingDdVars)) {
            return clusterIndex;
        }
    }
    return DUMMY_MAX_INT;
}

void NonlinearCounter::constructJoinTreeUsingListClustering(const Pbf& pbf, bool usingMinVar) {
    vector<Int> cnfVarOrdering = pbf.getVarOrdering(cnfVarOrderingHeuristic, inverseCnfVarOrdering);
    const vector<vector<Int>>& clauses = pbf.getClauses();
    const vector<vector<Int>>& coefficients = pbf.getCoefficients();
    const vector<string>& options = pbf.getOptions();
    const vector<Int>& limits = pbf.getLimits();

    fillClusters(clauses, cnfVarOrdering, usingMinVar);
    if (verbosityLevel >= 2)
        printClusters(clauses, coefficients, options, limits);

    fillCnfVarSets(clauses, usingMinVar);
    if (verbosityLevel >= 2) {
        printOccurrentCnfVarSets();
        printProjectableCnfVarSets();
    }

    vector<JoinNode*> terminals;
    for (Int clauseIndex = 0; clauseIndex < clauses.size(); clauseIndex++) {
        terminals.push_back(new JoinTerminal());
    }

    /* creates cluster nodes: */
    vector<JoinNonterminal*> clusterNodes(clusters.size(), nullptr);  // null node for empty cluster
    for (Int clusterIndex = 0; clusterIndex < clusters.size(); clusterIndex++) {
        const vector<Int>& clauseIndices = clusters[clusterIndex];
        if (!clauseIndices.empty()) {
            vector<JoinNode*> children;
            for (Int clauseIndex : clauseIndices) {
                children.push_back(terminals[clauseIndex]);
            }
            clusterNodes[clusterIndex] = new JoinNonterminal(children);
        }
    }

    Int nonNullClusterNodeIndex = 0;
    while (clusterNodes[nonNullClusterNodeIndex] == nullptr) {
        nonNullClusterNodeIndex++;
    }
    JoinNonterminal* nonterminal = clusterNodes[nonNullClusterNodeIndex];
    nonterminal->addProjectableCnfVars(projectableCnfVarSets[nonNullClusterNodeIndex]);
    joinRoot = nonterminal;

    for (Int clusterIndex = nonNullClusterNodeIndex + 1; clusterIndex < clusters.size(); clusterIndex++) {
        JoinNonterminal* clusterNode = clusterNodes[clusterIndex];
        if (clusterNode != nullptr) {
            joinRoot = new JoinNonterminal({joinRoot, clusterNode}, projectableCnfVarSets[clusterIndex]);
        }
    }
}

void NonlinearCounter::constructJoinTreeUsingTreeClustering(const Pbf& pbf, bool usingMinVar) {
    vector<Int> cnfVarOrdering = pbf.getVarOrdering(cnfVarOrderingHeuristic, inverseCnfVarOrdering);
    const vector<vector<Int>>& clauses = pbf.getClauses();
    const vector<vector<Int>>& coefficients = pbf.getCoefficients();
    const vector<string>& options = pbf.getOptions();
    const vector<Int>& limits = pbf.getLimits();

    fillClusters(clauses, cnfVarOrdering, usingMinVar);
    if (verbosityLevel >= 2)
        printClusters(clauses, coefficients, options, limits);

    fillCnfVarSets(clauses, usingMinVar);
    if (verbosityLevel >= 2) {
        printOccurrentCnfVarSets();
        printProjectableCnfVarSets();
    }

    vector<JoinNode*> terminals;
    for (Int clauseIndex = 0; clauseIndex < clauses.size(); clauseIndex++) {
        terminals.push_back(new JoinTerminal());
    }

    Int clusterCount = clusters.size();
    joinNodeSets = vector<vector<JoinNode*>>(clusterCount, vector<JoinNode*>());  // clusterIndex -> non-null nodes

    /* creates cluster nodes: */
    for (Int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++) {
        const vector<Int>& clauseIndices = clusters[clusterIndex];
        if (!clauseIndices.empty()) {
            vector<JoinNode*> children;
            for (Int clauseIndex : clauseIndices) {
                children.push_back(terminals[clauseIndex]);
            }
            joinNodeSets[clusterIndex].push_back(new JoinNonterminal(children));
        }
    }

    vector<JoinNode*> rootChildren;
    for (Int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++) {
        if (joinNodeSets[clusterIndex].empty())
            continue;

        Set<Int> projectableCnfVars = projectableCnfVarSets[clusterIndex];

        Set<Int> remainingCnfVars;
        util::differ(remainingCnfVars, occurrentCnfVarSets[clusterIndex], projectableCnfVars);
        occurrentCnfVarSets[clusterIndex] = remainingCnfVars;

        Int targetClusterIndex = getTargetClusterIndex(clusterIndex);
        if (targetClusterIndex <= clusterIndex) {
            showError("targetClusterIndex == " + to_string(targetClusterIndex) + " <= clusterIndex == " + to_string(clusterIndex));
        } else if (targetClusterIndex < clusterCount) {  // some var remains
            util::unionize(occurrentCnfVarSets[targetClusterIndex], remainingCnfVars);

            JoinNonterminal* nonterminal = new JoinNonterminal(joinNodeSets[clusterIndex], projectableCnfVars);
            joinNodeSets[targetClusterIndex].push_back(nonterminal);
        } else if (targetClusterIndex < DUMMY_MAX_INT) {
            showError("clusterCount <= targetClusterIndex < DUMMY_MAX_INT");
        } else {  // no var remains
            JoinNonterminal* nonterminal = new JoinNonterminal(joinNodeSets[clusterIndex], projectableCnfVars);
            rootChildren.push_back(nonterminal);
        }
    }
    joinRoot = new JoinNonterminal(rootChildren);
}

Number NonlinearCounter::countUsingListClustering(const Pbf& pbf, bool usingMinVar) {
    if (verbosityLevel >= 2) util::printComment("Call counting using List Clustering");
    orderDdVars(pbf);

    vector<Int> cnfVarOrdering = pbf.getVarOrdering(cnfVarOrderingHeuristic, inverseCnfVarOrdering);
    const vector<vector<Int>>& clauses = pbf.getClauses();  // 这里是用 const 的引用 应用了 clauses
    const vector<vector<Int>>& coefficents = pbf.getCoefficients();
    const vector<string> options = pbf.getOptions();
    const vector<Int> limits = pbf.getLimits();

    fillClusters(clauses, cnfVarOrdering, usingMinVar);  // cluster - 满足某种条件的 clause 组成的集合
    if (verbosityLevel >= 2) printClusters(clauses, coefficents, options, limits);

    /* builds ADD for CNF: */
    // ADD cnfDd = mgr.addOne();
    Dd cnfDd = Dd::getOneDd(mgr);
    Set<Int> projectedCnfVars;
    for (Int clusterIndex = 0; clusterIndex < clusters.size(); clusterIndex++) {
        /* builds ADD for cluster: */
        // ADD clusterDd = mgr.addOne();
        Dd clusterDd = Dd::getOneDd(mgr);
        const vector<Int>& clauseIndices = clusters[clusterIndex];

        if (verbosityLevel >= 2) util::printComment("Start to build cluster " + to_string(clusterIndex));
        for (Int clauseIndex : clauseIndices) {
            if (verbosityLevel >= 2) {
                util::printComment("Start to build constraint");
                util::printConstraint(clauses[clauseIndex], coefficents[clauseIndex], options[clauseIndex], limits[clauseIndex]);

                vector<Int> clause = clauses[clauseIndex];
                vector<Int> coefficent = coefficents[clauseIndex];
                string option = options[clauseIndex];
                Int limit = limits[clauseIndex];
                
                for (int i = 0; i < clause.size(); i++) {
                    if(clause[i] > 0)
                        cout << std::right << std::setw(5) << coefficent[i] << " x" << cnfVarToDdVarMap[clause[i]] << " ";
                    else 
                        cout << std::right << std::setw(5) << coefficent[i] << " x-" << cnfVarToDdVarMap[-clause[i]] << " ";
                }
                cout << std::right << std::setw(10) << option << " " << limit;
                cout << "\n";
            }
            Dd clauseDd = getConstraintDd(clauses[clauseIndex], coefficents[clauseIndex], options[clauseIndex], limits[clauseIndex]);

            if(verbosityLevel >= 4) {
                std::cout << "construct clauseDd to => " + to_string(dotFileIndex) + ".dot" << std::endl;
                writeDotFile(clauseDd, DOT_DIR);
            }
            

            // util::printComment("Call clusterDd *= clauseDd");
            // clusterDd *= clauseDd;  // 乘操作得到了什么？

            clusterDd = clusterDd.getProduct(clauseDd);

            if(verbosityLevel >= 4) {
                std::cout << "after *= clauseDd,  ouput clusterDd to => " + to_string(dotFileIndex) + ".dot" << std::endl;
                writeDotFile(clusterDd);
            }
        }  // clusterDd 得到了每个 clause 的乘

        if(verbosityLevel >= 4) {
            std::cout << "after *= clauseDd loop,  ouput clusterDd to => " + to_string(dotFileIndex) + ".dot" << std::endl;
            writeDotFile(clusterDd);
        }

        cnfDd = cnfDd.getProduct(clusterDd);

        if(verbosityLevel >= 4) {
            std::cout << "after *= cluster ouput cnfDd to => " + to_string(dotFileIndex) + ".dot" << std::endl;
            writeDotFile(cnfDd, DOT_DIR);
        }

        Set<Int> projectingDdVars = getProjectingDdVars(clusterIndex, usingMinVar, cnfVarOrdering, clauses);
        abstractCube(cnfDd, projectingDdVars, pbf.getLiteralWeights());
        util::unionize(projectedCnfVars, getCnfVars(projectingDdVars));  // add projecting to projected
    }

    // Float modelCount = diagram::countConstDdFloat(cnfDd);
    Number modelCount = cnfDd.extractConst();
    modelCount = util::adjustModelCount(modelCount, cnfVarOrdering, pbf.getLiteralWeights());
    return modelCount;
}

Number NonlinearCounter::countUsingTreeClustering(const Pbf& pbf, bool usingMinVar) {
    orderDdVars(pbf);

    vector<Int> cnfVarOrdering = pbf.getVarOrdering(cnfVarOrderingHeuristic, inverseCnfVarOrdering);
    const vector<vector<Int>>& clauses = pbf.getClauses();
    const vector<vector<Int>>& coefficients = pbf.getCoefficients();
    const vector<string>& options = pbf.getOptions();
    const vector<Int>& limits = pbf.getLimits();

    fillProjectingDdVarSets(clauses, coefficients, options, limits, cnfVarOrdering, usingMinVar);

    /* builds ADD for CNF: */
    // ADD cnfDd = mgr.addOne();
    Dd cnfDd = Dd::getOneDd(mgr);
    Set<Int> projectedCnfVars;
    Int clusterCount = clusters.size();
    for (Int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++) {
        const vector<Dd>& ddCluster = ddClusters[clusterIndex];
        if (!ddCluster.empty()) {
            /* builds ADD for cluster: */
            // ADD clusterDd = mgr.addOne();
            Dd clusterDd = Dd::getOneDd(mgr);
            for (const Dd& dd : ddCluster)
                // clusterDd *= dd;
                clusterDd = clusterDd.getProduct(dd);

            Set<Int> projectingDdVars = projectingDdVarSets[clusterIndex];
            if (usingMinVar && projectingDdVars.size() != 1)
                showError("wrong number of projecting vars (bucket elimination)");

            abstractCube(clusterDd, projectingDdVars, pbf.getLiteralWeights());
            util::unionize(projectedCnfVars, getCnfVars(projectingDdVars));

            Int newClusterIndex = getNewClusterIndex(clusterDd, cnfVarOrdering, usingMinVar);
            if (newClusterIndex <= clusterIndex) {
                showError("newClusterIndex == " + to_string(newClusterIndex) + " <= clusterIndex == " + to_string(clusterIndex));
            } else if (newClusterIndex < clusterCount) {  // some var remains
                ddClusters[newClusterIndex].push_back(clusterDd);
            } else if (newClusterIndex < DUMMY_MAX_INT) {
                showError("clusterCount <= newClusterIndex < DUMMY_MAX_INT");
            } else {  // no var remains
                // cnfDd *= clusterDd;
                cnfDd = cnfDd.getProduct(clusterDd);
            }
        }
    }

    // Float modelCount = diagram::countConstDdFloat(cnfDd);
    Number modelCount = cnfDd.extractConst();
    modelCount = util::adjustModelCount(modelCount, projectedCnfVars, pbf.getLiteralWeights());
    return modelCount;
}

/* class BucketCounter ********************************************************/

void BucketCounter::constructJoinTree(const Pbf& pbf) {
    bool usingMinVar = true;
    return usingTreeClustering ? NonlinearCounter::constructJoinTreeUsingTreeClustering(pbf, usingMinVar) : NonlinearCounter::constructJoinTreeUsingListClustering(pbf, usingMinVar);
}

Number BucketCounter::computeModelCount(const Pbf& pbf) {
    bool usingMinVar = true;
    return usingTreeClustering ? NonlinearCounter::countUsingTreeClustering(pbf, usingMinVar) : NonlinearCounter::countUsingListClustering(pbf, usingMinVar);
}

BucketCounter::BucketCounter(bool usingTreeClustering, VarOrderingHeuristic cnfVarOrderingHeuristic, bool inverseCnfVarOrdering, VarOrderingHeuristic ddVarOrderingHeuristic, bool inverseDdVarOrdering) {
    this->usingTreeClustering = usingTreeClustering;
    this->cnfVarOrderingHeuristic = cnfVarOrderingHeuristic;
    this->inverseCnfVarOrdering = inverseCnfVarOrdering;
    this->ddVarOrderingHeuristic = ddVarOrderingHeuristic;
    this->inverseDdVarOrdering = inverseDdVarOrdering;
}

/* class BouquetCounter *******************************************************/

void BouquetCounter::constructJoinTree(const Pbf& pbf) {
    bool usingMinVar = false;
    return usingTreeClustering ? NonlinearCounter::constructJoinTreeUsingTreeClustering(pbf, usingMinVar) : NonlinearCounter::constructJoinTreeUsingListClustering(pbf, usingMinVar);
}

Number BouquetCounter::computeModelCount(const Pbf& pbf) {
    bool usingMinVar = false;
    return usingTreeClustering ? NonlinearCounter::countUsingTreeClustering(pbf, usingMinVar) : NonlinearCounter::countUsingListClustering(pbf, usingMinVar);
}

BouquetCounter::BouquetCounter(bool usingTreeClustering, VarOrderingHeuristic cnfVarOrderingHeuristic, bool inverseCnfVarOrdering, VarOrderingHeuristic ddVarOrderingHeuristic, bool inverseDdVarOrdering) {
    this->usingTreeClustering = usingTreeClustering;
    this->cnfVarOrderingHeuristic = cnfVarOrderingHeuristic;
    this->inverseCnfVarOrdering = inverseCnfVarOrdering;
    this->ddVarOrderingHeuristic = ddVarOrderingHeuristic;
    this->inverseDdVarOrdering = inverseDdVarOrdering;
}