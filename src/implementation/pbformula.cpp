/* pb formula */

/* inclusions *****************************************************************/

#include "../interface/pbformula.hpp"

/* constants ******************************************************************/

const string &WEIGHT_WORD = "w";

/* classes ********************************************************************/

/* class Label ****************************************************************/

void PbLabel::addNumber(Int i) {
    push_back(i);
    std::sort(begin(), end(), std::greater<Int>());
}

/* class Pbf ******************************************************************/

void Pbf::updateApparentVars(Int literal) {
    Int var = util::getPbfVar(literal);
    apparentVarCount = var > apparentVarCount ? var : apparentVarCount;
    if (!util::isFound(var, apparentVars))
        apparentVars.push_back(var);
}

void Pbf::addConstraint(const vector<Int>& clause, const vector<Int>& coefficient, const string& option, const Int& limit) {
    clauses.push_back(clause);
    coefficients.push_back(coefficient);
    options.push_back(option);
    limits.push_back(limit);

    for (Int literal : clause) {
        updateApparentVars(literal);
    }
}

Graph Pbf::getGaifmanGraph() const {
    Set<Int> vars;
    for (Int var : apparentVars)
        vars.insert(var);
    Graph graph(vars);

    for (const vector<Int>& clause : clauses)
        for (auto literal1 = clause.begin(); literal1 != clause.end(); literal1++)
            for (auto literal2 = std::next(literal1); literal2 != clause.end(); literal2++) {
                Int var1 = util::getPbfVar(*literal1);
                Int var2 = util::getPbfVar(*literal2);
                graph.addEdge(var1, var2);
            }

    return graph;
}

vector<Int> Pbf::getAppearanceVarOrdering() const {
    return apparentVars;
}

Int Pbf::getApparentVarCount() const {
    return apparentVarCount;
}

/** Declaration Ordering, x1, x2, x3, etc **/
vector<Int> Pbf::getDeclarationVarOrdering() const {
    vector<Int> varOrdering = apparentVars;
    std::sort(varOrdering.begin(), varOrdering.end());
    return varOrdering;
}

vector<Int> Pbf::getRandomVarOrdering() const {
    vector<Int> varOrdering = apparentVars;
    util::shuffleRandomly(varOrdering);
    return varOrdering;
}

/* same as Cnf class */
vector<Int> Pbf::getLexpVarOrdering() const {
    Map<Int, PbLabel> unnumberedVertices;
    for (Int vertex : apparentVars)
        unnumberedVertices[vertex] = PbLabel();
    vector<Int> numberedVertices;  // whose \alpha numbers are decreasing
    Graph graph = getGaifmanGraph();
    for (Int number = apparentVars.size(); number > 0; number--) {
        auto vertexIt = std::max_element(unnumberedVertices.begin(),
                                         unnumberedVertices.end(), util::isLessValued<Int, PbLabel>);
        Int vertex = vertexIt->first;  // ignores label
        numberedVertices.push_back(vertex);
        unnumberedVertices.erase(vertex);
        for (auto neighborIt = graph.beginNeighbors(vertex);
             neighborIt != graph.endNeighbors(vertex); neighborIt++) {
            Int neighbor = *neighborIt;
            auto unnumberedNeighborIt = unnumberedVertices.find(neighbor);
            if (unnumberedNeighborIt != unnumberedVertices.end()) {
                Int unnumberedNeighbor = unnumberedNeighborIt->first;
                unnumberedVertices.at(unnumberedNeighbor).addNumber(number);
            }
        }
    }
    return numberedVertices;
}

vector<Int> Pbf::getLexmVarOrdering() const {
    Map<Int, PbLabel> unnumberedVertices;
    for (Int vertex : apparentVars)
        unnumberedVertices[vertex] = PbLabel();
    vector<Int> numberedVertices;  // whose \alpha numbers are decreasing
    Graph graph = getGaifmanGraph();
    for (Int i = apparentVars.size(); i > 0; i--) {
        auto vIt = std::max_element(unnumberedVertices.begin(),
                                    unnumberedVertices.end(), util::isLessValued<Int, PbLabel>);
        Int v = vIt->first;  // ignores label
        numberedVertices.push_back(v);
        unnumberedVertices.erase(v);

        /* updates numberedVertices: */
        Graph subgraph = getGaifmanGraph();  // will only contain v, w, and unnumbered vertices whose labels are less than w's
        for (auto wIt = unnumberedVertices.begin(); wIt != unnumberedVertices.end(); wIt++) {
            Int w = wIt->first;
            PbLabel& wLabel = wIt->second;

            /* removes numbered vertices except v: */
            for (Int numberedVertex : numberedVertices)
                if (numberedVertex != v)
                    subgraph.removeVertex(numberedVertex);

            /* removes each non-w unnumbered vertex whose label is not less than w's */
            for (const std::pair<Int, PbLabel>& kv : unnumberedVertices) {
                Int unnumberedVertex = kv.first;
                const PbLabel& label = kv.second;
                if (unnumberedVertex != w && label >= wLabel)
                    subgraph.removeVertex(unnumberedVertex);
            }

            if (subgraph.hasPath(v, w))
                wLabel.addNumber(i);
        }
    }
    return numberedVertices;
}

vector<Int> Pbf::getMcsVarOrdering() const {
    Graph graph = getGaifmanGraph();

    auto startVertex = graph.beginVertices();
    if (startVertex == graph.endVertices())  // empty graph
        return vector<Int>();

    Map<Int, Int> rankedNeighborCounts;  // unranked vertex |-> number of ranked neighbors
    for (auto it = std::next(startVertex); it != graph.endVertices(); it++)
        rankedNeighborCounts[*it] = 0;

    Int bestVertex = *startVertex;
    Int bestRankedNeighborCount = DUMMY_MIN_INT;

    vector<Int> varOrdering;
    do {
        varOrdering.push_back(bestVertex);

        rankedNeighborCounts.erase(bestVertex);

        for (auto n = graph.beginNeighbors(bestVertex); n != graph.endNeighbors(bestVertex); n++) {
            auto entry = rankedNeighborCounts.find(*n);
            if (entry != rankedNeighborCounts.end())
                entry->second++;
        }

        bestRankedNeighborCount = DUMMY_MIN_INT;
        for (const std::pair<Int, Int>& entry : rankedNeighborCounts)
            if (entry.second > bestRankedNeighborCount) {
                bestRankedNeighborCount = entry.second;
                bestVertex = entry.first;
            }
    } while (bestRankedNeighborCount != DUMMY_MIN_INT);

    return varOrdering;
}

/* Public functions */

vector<Int> Pbf::getVarOrdering(VarOrderingHeuristic varOrderingHeuristic, bool inverse) const {
    vector<Int> varOrdering;             // make varOrdering
    switch (varOrderingHeuristic) {
        case VarOrderingHeuristic::APPEARANCE: {
            varOrdering = getAppearanceVarOrdering();
            break;
        }
        case VarOrderingHeuristic::DECLARATION: {
            varOrdering = getDeclarationVarOrdering();
            break;
        }
        case VarOrderingHeuristic::RANDOM: {
            varOrdering = getRandomVarOrdering();
            break;
        }
        case VarOrderingHeuristic::LEXP: {
            varOrdering = getLexpVarOrdering();
            break;
        }
        case VarOrderingHeuristic::LEXM: {
            varOrdering = getLexmVarOrdering();
            break;
        }
        case VarOrderingHeuristic::MCS: {
            varOrdering = getMcsVarOrdering();
            break;
        }
        default: {
            showError("DUMMY_VAR_ORDERING_HEURISTIC -- Pbf::getVarOrdering");
        }
    }
    if (inverse) {
        util::invert(varOrdering);
    }
    return varOrdering;
}

Int Pbf::getDeclaredVarCount() const {
    return declaredVarCount;
}

const vector<Int>& Pbf::getApparentVars() const {
    return apparentVars;
}

Map<Int, Number> Pbf::getLiteralWeights() const {
    return literalWeights; 
}

Int Pbf::getEmptyClauseIndex() const {
    for (Int clauseIndex = 0; clauseIndex < clauses.size(); clauseIndex++) {
        if (clauses.at(clauseIndex).empty()) {
            return clauseIndex;
        }
    }
    return DUMMY_MIN_INT;
}

const vector<vector<Int>>& Pbf::getClauses() const {
    return clauses;
}

const vector<vector<Int>>& Pbf::getCoefficients() const {
    return coefficients;
}

const vector<string>& Pbf::getOptions() const {
    return options;
}

const vector<Int>& Pbf::getLimits() const {
    return limits;
}

void Pbf::clearConstraints() {
    clauses.clear();
    coefficients.clear();
    options.clear();
    limits.clear();
}

void Pbf::setClauses(vector<vector<Int> > clauses) {
    this->clauses = clauses;
}

void Pbf::setCoefficients(vector<vector<Int> > coef) {
    this->coefficients = coef;
}

void Pbf::setOptions(vector<string> options) {
    this->options = options;
}

void Pbf::setLimits(vector<Int> limits) {
    this->limits = limits;
}

void Pbf::printConstraints() const {
    util::printPbf(clauses, coefficients, options, limits);
}

Pbf::Pbf(const string& filePath, WeightFormat weightFormat) {
    printComment("Reading PBF formula...", 1);

    std::ifstream inputFileStream(filePath);  // variable will be destroyed if it goes out of scope
    std::istream* inputStream;
    if (filePath == STDIN_CONVENTION) {
        inputStream = &std::cin;

        printThickLine();
        printComment("Getting cnf from stdin... (end input with 'Enter' then 'Ctrl d')");
    } else {
        if (!inputFileStream.is_open()) {
            showError("unable to open file '" + filePath + "'");
        }
        inputStream = &inputFileStream;
    }
    Int declaredConstraintCount = DUMMY_MIN_INT;
    Int processedConstraintCount = 0;
    this->weightFormat = weightFormat;

    Int lineIndex = 0;

    string line;
    while (std::getline(*inputStream, line)) {
        lineIndex++;
        std::istringstream inputStringStream(line);

        if (verbosityLevel >= 4) printComment("Line " + to_string(lineIndex) + "\t" + line);

        vector<string> words;
        std::copy(std::istream_iterator<string>(inputStringStream), std::istream_iterator<string>(), std::back_inserter(words));

        Int wordCount = words.size();
        if (wordCount < 1) continue;

        bool endLineFlag = false;
        const string& startWord = words.at(0);
        
        // judge "min:" for opb
        if (startWord == COMMENT_WORD || startWord.at(0) == COMMENT_WORD.at(0) || startWord == "min:") {  // "*"
            if(wordCount < 5) continue;
            const string &declareV = words.at(1);
            if (declareV == COMMENT_VARIABLE_WORD) declaredVarCount = std::stoll(words.at(2));
            const string &declareC = words.at(3);
            if (declareC == COMMENT_CONSTRAINT_WORD) declaredConstraintCount = std::stoll(words.at(4));
        } else if(startWord == WEIGHT_WORD) { // weight line
            if(weightFormat == WeightFormat::UNWEIGHTED) continue;
                // util::showError("Wrong weighted option");
            if(wordCount == 3) {
                string var = words.at(1);                 // now i = i+1
                if (var.at(0) != VARIABLE_WORD) showError("Wrong Variable format");
                Int literal = std::stoll(var.substr(1));
                // Float weight = std::stod(words.at(2));
                literalWeights[literal] = Number(words.at(2));
            } else {
                util::showWarning("Wrong weight format");
            }
        } else {  // clause line
            vector<Int> clause;
            vector<Int> coefficient;
            Int limit;
            for (Int i = 0; i < wordCount; i++) {
                if(endLineFlag && i != wordCount - 1) showError("External words after relation limit " + words.at(i));

                const string &nowWord = words.at(i);
                if(nowWord == EQUAL_WORD) {                     // == only need format
                    // limit = std::stoll(words.at(++i));          // now i = i+1
                    // util::formatConstraint(clause, coefficient, limit);
                    // addConstraint(clause, coefficient, limit);
                    // util::inverseConstraint(clause, coefficient, limit);
                    // addConstraint(clause, coefficient, limit);
                    // endLineFlag = true;

                    limit = std::stoll(words.at(++i));          // now i = i+1
                    util::formatConstraint(clause, coefficient, limit);
                    addConstraint(clause, coefficient, EQUAL_WORD, limit); // !! == option
                    endLineFlag = true;
                } else if(nowWord == GEQUAL_WORD) {             // >= need inverse
                    limit = std::stoll(words.at(++i));          // now i = i+1
                    util::inverseConstraint(clause, coefficient, limit);
                    addConstraint(clause, coefficient, LEQUAL_WORD, limit);     // inverse to <=
                    endLineFlag = true;
                } else if(nowWord == LEQUAL_WORD) {             // <= need format
                    limit = std::stoll(words.at(++i));          // now i = i+1
                    util::formatConstraint(clause, coefficient, limit);
                    addConstraint(clause, coefficient, LEQUAL_WORD, limit);
                    endLineFlag = true;
                } else if(nowWord == END_LINE_WORD){
                    if(!endLineFlag) showError("end line without completion constraint");
                    processedConstraintCount++;
                } else {                                        // coefficient & variable
                    Int coef = std::stoll(words.at(i));
                    string var = words.at(++i);                 // now i = i+1
                    if (var.at(0) != VARIABLE_WORD) showError("Wrong Variable format");
                    Int literal = std::stoll(var.substr(1));
                    // cout << "Var " << var << " substr " << var.substr(1) << " literal " << literal << std::endl;
                    if (literal > declaredVarCount || literal < -declaredVarCount) {
                        showError("literal '" + to_string(literal) + "' is inconsistent with declared var count '" + to_string(declaredVarCount) + "' -- line " + to_string(lineIndex));
                    }
                    clause.push_back(literal);
                    coefficient.push_back(coef);
                }
            }
        }
    }

    if (weightFormat == WeightFormat::UNWEIGHTED) { // populates literalWeights with 1s
        for (Int var = 1; var <= declaredVarCount; var++) {
            literalWeights[var] = Number("1");
            literalWeights[-var] = Number("1");
        }
    }

    if (filePath == STDIN_CONVENTION) {
        printComment("Getting cnf from stdin: done");
        printThickLine();
    }

    if (verbosityLevel >= 1) {
        util::printRow("declaredVarCount", declaredVarCount);
        util::printRow("apparentVarCount", apparentVars.size());
        util::printRow("declaredClauseCount", declaredConstraintCount);
        util::printRow("apparentClauseCount", processedConstraintCount);
    }

    if (verbosityLevel >= 3) {
        printConstraints();
    }
}

Pbf::Pbf(const vector<vector<Int>>& clauses, const vector<vector<Int>> &coefficients, const vector<string> &options, const vector<Int> &limits) {
    this->clauses = clauses;
    this->coefficients = coefficients;
    this->options = options;
    this->limits = limits;

    for (const vector<Int>& clause : clauses) {
        for (Int literal : clause) {
            updateApparentVars(literal);
        }
    }
}
