/* inclusions *****************************************************************/

#include "../interface/util.hpp"

/* constants ******************************************************************/

const Float MEGA = 1e6l;

// const string &COMMENT_WORD = "c";    // cnf comment word
const string& COMMENT_WORD = "*";  // pbf comment word
const string& PROBLEM_WORD = "p";
const char& VARIABLE_WORD = 'x';

const string& COMMENT_VARIABLE_WORD = "#variable=";
const string& COMMENT_CONSTRAINT_WORD = "#constraint=";

const string& EQUAL_WORD = "=";
const string& GEQUAL_WORD = ">=";
const string& LEQUAL_WORD = "<=";

const string& END_LINE_WORD = ";";

const string& STDIN_CONVENTION = "-";

const string& REQUIRED_OPTION_GROUP = "Required";
const string& OPTIONAL_OPTION_GROUP = "Optional";

const string& HELP_OPTION = "h, help";
const string& INPUT_FILE_OPTION = "if";
const string& WEIGHT_FORMAT_OPTION = "wf";
const string& CLUSTERING_HEURISTIC_OPTION = "ch";
const string& CLUSTER_VAR_ORDER_OPTION = "cv";
const string& DIAGRAM_VAR_ORDER_OPTION = "dv";
const string& RANDOM_SEED_OPTION = "rs";
const string& VERBOSITY_LEVEL_OPTION = "vl";
const string& DIAGRAM_PACKAGE_OPTION = "dp";
const string& MULTIPLE_PRECISION_OPTION = "mp";
const string& MAXIMUM_MEMORY_OPTION = "mm";
const string& PREPROCESSOR_OPTION = "pr";

const string& CUDD_PACKAGE = "c";
const string& SYLVAN_PACKAGE = "s";

const string DEFAULT_DIAGRAM_PACKAGE = "s";
const Int DEFAULT_MULTIPLE_PRECISION = 1;
const Int DEFAULT_MAXIMUM_MEMORY = 16000;
const Int DEFAULT_PREPROCESSOR = 0;

/* global variables ***********************************************************/

Int randomSeed = DEFAULT_RANDOM_SEED;
Int verbosityLevel = DEFAULT_VERBOSITY_LEVEL_CHOICE;
TimePoint startTime;
bool multiplePrecision = DEFAULT_MULTIPLE_PRECISION;
string ddPackage = DEFAULT_DIAGRAM_PACKAGE;
Float memSensitivity;
Int dotFileIndex = 1;  // dotFileIndex 输出 dot 文件 write 后会自加
Int maxMem = DEFAULT_MAXIMUM_MEMORY;
Float tableRatio = 1;
Float initRatio = 10;
bool preprocessorFlag = DEFAULT_PREPROCESSOR;

const std::map<Int, WeightFormat> WEIGHT_FORMAT_CHOICES = {
    {1, WeightFormat::UNWEIGHTED},
    {2, WeightFormat::WEIGHTED}};
const Int DEFAULT_WEIGHT_FORMAT_CHOICE = 2;

const std::map<Int, ClusteringHeuristic> CLUSTERING_HEURISTIC_CHOICES = {
    {1, ClusteringHeuristic::MONOLITHIC},
    {2, ClusteringHeuristic::LINEAR},
    {3, ClusteringHeuristic::BUCKET_LIST},
    {4, ClusteringHeuristic::BUCKET_TREE},
    {5, ClusteringHeuristic::BOUQUET_LIST},
    {6, ClusteringHeuristic::BOUQUET_TREE}};
const Int DEFAULT_CLUSTERING_HEURISTIC_CHOICE = 6;

const std::map<Int, VarOrderingHeuristic> VAR_ORDERING_HEURISTIC_CHOICES = {
    {1, VarOrderingHeuristic::APPEARANCE},
    {2, VarOrderingHeuristic::DECLARATION},
    {3, VarOrderingHeuristic::RANDOM},
    {4, VarOrderingHeuristic::MCS},
    {5, VarOrderingHeuristic::LEXP},
    {6, VarOrderingHeuristic::LEXM}};
const Int DEFAULT_CNF_VAR_ORDERING_HEURISTIC_CHOICE = 5;
const Int DEFAULT_DD_VAR_ORDERING_HEURISTIC_CHOICE = 4;

const Int DEFAULT_RANDOM_SEED = 10;

const vector<Int> VERBOSITY_LEVEL_CHOICES = {0, 1, 2, 3, 4};
const Int DEFAULT_VERBOSITY_LEVEL_CHOICE = 1;

const Float NEGATIVE_INFINITY = -std::numeric_limits<Float>::infinity();

const Int DUMMY_MIN_INT = std::numeric_limits<Int>::min();
const Int DUMMY_MAX_INT = std::numeric_limits<Int>::max();

const string& DUMMY_STR = "";

const string& DOT_DIR = "./dot/";

/* namespaces *****************************************************************/

/* namespace util *************************************************************/

bool util::isInt(Float d) {
    Float intPart;
    Float fractionalPart = modf(d, &intPart);
    return fractionalPart == 0.0;
}

/* functions: printing ********************************************************/

void util::printComment(const string& message, Int preceedingNewLines, Int followingNewLines, bool commented) {
    for (Int i = 0; i < preceedingNewLines; i++)
        cout << "\n";
    cout << (commented ? COMMENT_WORD + " " : "") << message;
    for (Int i = 0; i < followingNewLines; i++)
        cout << "\n";
}

void util::printSolutionLine(Number modelCount, WeightFormat weightFormat, Int preceedingThinLines, Int followingThinLines) {
    for (Int i = 0; i < preceedingThinLines; i++)
        printThinLine();

    // format output
    if(weightFormat == WeightFormat::UNWEIGHTED) cout << std::fixed << std::setprecision(0);

    // cout << "s " << " " << modelCount << "\n"
    if(multiplePrecision) {
        cout << "s  " << mpf_class(modelCount.quotient) << "\n";
    } else {
        cout << "s  " << modelCount << "\n";
    }

    for (Int i = 0; i < followingThinLines; i++)
        printThinLine();
}

void util::printCnfSolutionLine(WeightFormat weightFormat, Number modelCount, Int preceedingThinLines, Int followingThinLines) {
    for (Int i = 0; i < preceedingThinLines; i++)
        printThinLine();
    cout << "s " << (weightFormat == WeightFormat::UNWEIGHTED ? "mc" : "wmc") << " " << modelCount << "\n";
    for (Int i = 0; i < followingThinLines; i++)
        printThinLine();
}

void util::printBoldLine(bool commented) {
    printComment("******************************************************************", 0, 1, commented);
}

void util::printThickLine(bool commented) {
    printComment("==================================================================", 0, 1, commented);
}

void util::printThinLine() {
    printComment("------------------------------------------------------------------");
}

void util::printHelpOption() {
    cout << "  -h, --help    help information\n";
}

void util::printCnfFileOption() {
    cout << "      --" << INPUT_FILE_OPTION << std::left << std::setw(56) << " arg  opb file path" << std::endl;
}

void util::printWeightFormatOption() {
    cout << "      --" << WEIGHT_FORMAT_OPTION << " arg  ";
    cout << "weight format in opb file:\n";
    for (const auto& kv : WEIGHT_FORMAT_CHOICES) {
        int num = kv.first;
        cout << "           " << num << "    " << std::left << std::setw(50) << getWeightFormatName(kv.second);
        if (num == DEFAULT_WEIGHT_FORMAT_CHOICE)
            cout << "Default: " << DEFAULT_WEIGHT_FORMAT_CHOICE;
        cout << "\n";
    }
}

void util::printClusteringHeuristicOption() {
    cout << "      --" << CLUSTERING_HEURISTIC_OPTION << " arg  ";
    cout << "clustering heuristic:\n";
    for (const auto& kv : CLUSTERING_HEURISTIC_CHOICES) {
        int num = kv.first;
        cout << "           " << num << "    " << std::left << std::setw(50) << getClusteringHeuristicName(kv.second);
        if (num == DEFAULT_CLUSTERING_HEURISTIC_CHOICE)
            cout << "Default: " << DEFAULT_CLUSTERING_HEURISTIC_CHOICE;
        cout << "\n";
    }
}

void util::printCnfVarOrderingHeuristicOption() {
    cout << "      --" << CLUSTER_VAR_ORDER_OPTION << " arg  ";
    cout << "cluster variable order heuristic (negate to invert):\n";
    for (const auto& kv : VAR_ORDERING_HEURISTIC_CHOICES) {
        int num = kv.first;
        cout << "           " << num << "    " << std::left << std::setw(50) << getVarOrderingHeuristicName(kv.second);
        if (num == std::abs(DEFAULT_CNF_VAR_ORDERING_HEURISTIC_CHOICE))
            cout << "Default: " << DEFAULT_CNF_VAR_ORDERING_HEURISTIC_CHOICE;
        cout << "\n";
    }
}

void util::printDdVarOrderingHeuristicOption() {
    cout << "      --" << DIAGRAM_VAR_ORDER_OPTION << " arg  ";
    cout << "diagram variable order heuristic (negate to invert):\n";
    for (const auto& kv : VAR_ORDERING_HEURISTIC_CHOICES) {
        int num = kv.first;
        cout << "           " << num << "    " << std::left << std::setw(50) << getVarOrderingHeuristicName(kv.second);
        if (num == std::abs(DEFAULT_DD_VAR_ORDERING_HEURISTIC_CHOICE))
            cout << "Default: " << DEFAULT_DD_VAR_ORDERING_HEURISTIC_CHOICE;
        cout << "\n";
    }
}

void util::printRandomSeedOption() {
    cout << "      --" << RANDOM_SEED_OPTION << std::left << std::setw(56) << " arg  random seed";
    cout << "Default: " + to_string(DEFAULT_RANDOM_SEED) + "\n";
}

void util::printVerbosityLevelOption() {
    cout << "      --" << VERBOSITY_LEVEL_OPTION << " arg  ";
    cout << "verbosity level:\n";
    for (Int verbosityLevelOption : VERBOSITY_LEVEL_CHOICES) {
        cout << "           " << verbosityLevelOption << "    " << std::left << std::setw(50) << getVerbosityLevelName(verbosityLevelOption);
        if (verbosityLevelOption == DEFAULT_VERBOSITY_LEVEL_CHOICE)
            cout << "Default: " << DEFAULT_VERBOSITY_LEVEL_CHOICE;
        cout << "\n";
    }
}

void util::printDiagramPackageOption() {
    cout << "      --" << DIAGRAM_PACKAGE_OPTION << " arg  ";
    cout << "diagram package:\n";
    string diagramPackageOption = CUDD_PACKAGE;
    cout << "           " << diagramPackageOption << "    " << std::left << std::setw(51) << "CUDD Package";
    cout << "\n";
    diagramPackageOption = SYLVAN_PACKAGE;
    cout << "           " << diagramPackageOption << "    " << std::left << std::setw(50) << "Sylvan Package";
    cout << "Default: " << DEFAULT_DIAGRAM_PACKAGE << "\n";
}

void util::printMultiplePrecisionOption() {
    cout << "      --" << MULTIPLE_PRECISION_OPTION << std::left << std::setw(56) << " arg  0/1; multiple prescision (1 need Sylvan Package)";
    cout << "Default: " + to_string(DEFAULT_MULTIPLE_PRECISION) + "\n";
}

void util::printMaximumMemoryOption() {
    cout << "      --" << MAXIMUM_MEMORY_OPTION << std::left << std::setw(56) << " arg  maximum memory";
    cout << "Default: " + to_string(DEFAULT_MAXIMUM_MEMORY) + "\n";
}

void util::printPreprocessorOption() {
    cout << "      --" << PREPROCESSOR_OPTION << std::left << std::setw(56) << " arg  0/1; backbone preprocessor";
    cout << "Default: " + to_string(DEFAULT_PREPROCESSOR) + "\n";
}

/* functions: argument parsing ************************************************/

vector<string> util::getArgV(int argc, char* argv[]) {
    vector<string> argV;
    for (Int i = 0; i < argc; i++)
        argV.push_back(string(argv[i]));
    return argV;
}

string util::getWeightFormatName(WeightFormat weightFormat) {
    switch (weightFormat) {
        case WeightFormat::UNWEIGHTED: {
            return "UNWEIGHTED";
        }
        case WeightFormat::WEIGHTED: {
            return "WEIGHTED";
        }
        default: {
            showError("no such weightFormat");
            return DUMMY_STR;
        }
    }
}


string util::getClusteringHeuristicName(ClusteringHeuristic clusteringHeuristic) {
    switch (clusteringHeuristic) {
        case ClusteringHeuristic::MONOLITHIC: {
            return "MONOLITHIC";
        }
        case ClusteringHeuristic::LINEAR: {
            return "LINEAR";
        }
        case ClusteringHeuristic::BUCKET_LIST: {
            return "BUCKET_LIST";
        }
        case ClusteringHeuristic::BUCKET_TREE: {
            return "BUCKET_TREE";
        }
        case ClusteringHeuristic::BOUQUET_LIST: {
            return "BOUQUET_LIST";
        }
        case ClusteringHeuristic::BOUQUET_TREE: {
            return "BOUQUET_TREE";
        }
        default: {
            showError("no such clusteringHeuristic");
            return DUMMY_STR;
        }
    }
}

string util::getVarOrderingHeuristicName(VarOrderingHeuristic varOrderingHeuristic) {
    switch (varOrderingHeuristic) {
        case VarOrderingHeuristic::APPEARANCE: {
            return "APPEARANCE";
        }
        case VarOrderingHeuristic::DECLARATION: {
            return "DECLARATION";
        }
        case VarOrderingHeuristic::RANDOM: {
            return "RANDOM";
        }
        case VarOrderingHeuristic::LEXP: {
            return "LEXP";
        }
        case VarOrderingHeuristic::LEXM: {
            return "LEXM";
        }
        case VarOrderingHeuristic::MCS: {
            return "MCS";
        }
        default: {
            showError("DUMMY_VAR_ORDERING_HEURISTIC in util::getVarOrderingHeuristicName");
            return DUMMY_STR;
        }
    }
}

string util::getVerbosityLevelName(Int verbosityLevel) {
    switch (verbosityLevel) {
        case 0: {
            return "solution only";
        }
        case 1: {
            return "parsed info as well";
        }
        case 2: {
            return "clusters as well";
        }
        case 3: {
            return "cnf literal weights as well";
        }
        case 4: {
            return "input lines as well";
        }
        default: {
            showError("no such verbosityLevel");
            return DUMMY_STR;
        }
    }
}

/* functions: CNF *************************************************************/

Int util::getCnfVar(Int literal) {
    if (literal == 0) {
        showError("literal is 0");
    }
    return std::abs(literal);
}

Set<Int> util::getClauseCnfVars(const vector<Int>& clause) {
    Set<Int> cnfVars;
    for (Int literal : clause)
        cnfVars.insert(getCnfVar(literal));
    return cnfVars;
}

Set<Int> util::getClusterCnfVars(const vector<Int>& cluster, const vector<vector<Int>>& clauses) {
    Set<Int> cnfVars;
    for (Int clauseIndex : cluster)
        unionize(cnfVars, getClauseCnfVars(clauses.at(clauseIndex)));
    return cnfVars;
}

bool util::appearsIn(Int cnfVar, const vector<Int>& clause) {
    for (Int literal : clause)
        if (getCnfVar(literal) == cnfVar)
            return true;
    return false;
}

bool util::isPositiveLiteral(Int literal) {
    if (literal == 0)
        showError("literal is 0");
    return literal > 0;
}

Int util::getLiteralRank(Int literal, const vector<Int>& cnfVarOrdering) {
    Int cnfVar = getCnfVar(literal);
    auto it = std::find(cnfVarOrdering.begin(), cnfVarOrdering.end(), cnfVar);
    if (it == cnfVarOrdering.end())
        showError("cnfVar not found in cnfVarOrdering");
    return it - cnfVarOrdering.begin();
}

Int util::getMinClauseRank(const vector<Int>& clause, const vector<Int>& cnfVarOrdering) {
    Int minRank = DUMMY_MAX_INT;
    for (Int literal : clause) {
        Int rank = getLiteralRank(literal, cnfVarOrdering);
        if (rank < minRank)
            minRank = rank;
    }
    return minRank;
}

Int util::getMaxClauseRank(const vector<Int>& clause, const vector<Int>& cnfVarOrdering) {
    Int maxRank = DUMMY_MIN_INT;
    for (Int literal : clause) {
        Int rank = getLiteralRank(literal, cnfVarOrdering);
        if (rank > maxRank)
            maxRank = rank;
    }
    return maxRank;
}

void util::printClause(const vector<Int>& clause) {
    for (Int literal : clause) {
        cout << std::right << std::setw(5) << literal << " ";
    }
    cout << "\n";
}

void util::printCnf(const vector<vector<Int>>& clauses) {
    printThinLine();
    printComment("cnf {");
    for (Int i = 0; i < clauses.size(); i++) {
        cout << COMMENT_WORD << "\t"
                                "clause ";
        cout << std::right << std::setw(5) << i + 1 << " : ";
        printClause(clauses.at(i));
    }
    printComment("}");
    printThinLine();
}

void util::printLiteralWeights(const Map<Int, Float>& literalWeights) {
    Int maxCnfVar = DUMMY_MIN_INT;
    for (const std::pair<Int, Float>& kv : literalWeights) {
        Int cnfVar = kv.first;
        if (cnfVar > maxCnfVar) {
            maxCnfVar = cnfVar;
        }
    }

    printThinLine();
    printComment("literalWeights {");
    cout << std::right;
    for (Int cnfVar = 1; cnfVar <= maxCnfVar; cnfVar++) {
        cout << COMMENT_WORD << " " << std::right << std::setw(10) << cnfVar << "\t" << std::left << std::setw(15) << literalWeights.at(cnfVar) << "\n";
        cout << COMMENT_WORD << " " << std::right << std::setw(10) << -cnfVar << "\t" << std::left << std::setw(15) << literalWeights.at(-cnfVar) << "\n";
    }
    printComment("}");
    printThinLine();
}

/* functions: PBF *************************************************************/
Int util::getPbfVar(Int literal) {
    if (literal == 0) {
        showError("literal is 0");
    }
    return std::abs(literal);
}

// now relation word is <= but may have negative coefficient
void util::formatConstraint(vector<Int>& clause, vector<Int>& coefficient, Int& limit) {
    for (int i = 0; i < coefficient.size(); i++) {
        const Int& coef = coefficient.at(i);
        if (coef > 0)
            continue;
        else if (coef < 0) {
            coefficient.at(i) = -coefficient.at(i);
            clause.at(i) = -clause.at(i);
            limit += coefficient.at(i);  // + positive number
        } else
            showError("Coefficient is 0");
    }
    if (limit < 0)
        showError("Formula <= negative limit");
}

// now relation word is >= need to inverse to <=
void util::inverseConstraint(vector<Int>& clause, vector<Int>& coefficient, Int& limit) {
    limit = -limit;
    for (int i = 0; i < coefficient.size(); i++) {
        const Int& coef = coefficient.at(i);
        if (coef > 0) {  // -coef < 0;
            clause.at(i) = -clause.at(i);
            limit += coefficient.at(i);
        } else if (coef < 0) {  // -coef > 0
            coefficient.at(i) = -coefficient.at(i);
        } else {
            showError("Coefficient is 0");
        }
    }
    if (limit < 0)
        showError("Formula <= negative limit");
}

void util::printConstraint(const vector<Int>& clause, const vector<Int>& coefficent, const string& option, const Int& limit) {
    for (int i = 0; i < clause.size(); i++) {
        cout << std::right << std::setw(3) << (coefficent.at(i) > 0 ? "+" : "") << coefficent.at(i) << " x" << std::left << std::setw(5) << clause.at(i) << " ";
    }
    cout << " " << std::right << std::setw(2) << option << " " << limit;
    cout << "\n";
}

void util::printPbf(const vector<vector<Int>>& clauses, const vector<vector<Int>>& coefficents, const vector<string> &options, const vector<Int>& limits) {
    Int clausesSize = clauses.size();
    Int coefficentsSize = coefficents.size();
    Int limitsSize = limits.size();

    if (clausesSize != coefficentsSize || clausesSize != limitsSize) {
        showError("Unpair Constraints Size");
    }

    printThinLine();
    printComment("pbf {");
    for (Int i = 0; i < clauses.size(); i++) {
        cout << COMMENT_WORD << "\t"
                                "Constraint ";
        cout << std::right << std::setw(5) << i + 1 << " : ";
        printConstraint(clauses.at(i), coefficents.at(i), options.at(i), limits.at(i));
    }
    printComment("}");
    printThinLine();

    // std::cout << "Not implement" << std::endl;
}
/* functions: timing **********************************************************/

TimePoint util::getTimePoint() {
    return std::chrono::steady_clock::now();
}

Float util::getSeconds(TimePoint startTime) {
    TimePoint endTime = getTimePoint();
    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count() / 1000.0;
}

void util::printDuration(TimePoint startTime) {
    printThickLine();
    printRow("seconds", getSeconds(startTime));
    printThickLine();
}

/* functions: error handling **************************************************/

void util::showWarning(const string& message, bool commented) {
    printBoldLine(commented);
    printComment("MY_WARNING: " + message, 0, 1, commented);
    printBoldLine(commented);
}

void util::showError(const string& message, bool commented) {
    throw MyError(message, commented);
}

/* classes ********************************************************************/

/* class MyError **************************************************************/

MyError::MyError(const string& message, bool commented) {
    util::printBoldLine(commented);
    util::printComment("MY_ERROR: " + message, 0, 1, commented);
    util::printBoldLine(commented);
}

/* class Number ============================================================= */

Number::Number(const mpq_class& q) {
    assert(multiplePrecision);
    quotient = q;
}

Number::Number(Float f) {
    assert(!multiplePrecision);
    fraction = f;
}

Number::Number(const Number& n) {
    if (multiplePrecision) {
        *this = Number(n.quotient);
    } else {
        *this = Number(n.fraction);
    }
}

Number::Number(const string& repr) {
    Int divPos = repr.find('/');
    if (multiplePrecision) {
        if (divPos != string::npos) {  // repr is <int>/<int>
            *this = Number(mpq_class(repr));
        } else {  // repr is <float>
            *this = Number(mpq_class(mpf_class(repr)));
        }
    } else {
        if (divPos != string::npos) {  // repr is <int>/<int>
            Float numerator = stold(repr.substr(0, divPos));
            Float denominator = stold(repr.substr(divPos + 1));
            *this = Number(numerator / denominator);
        } else {  // repr is <float>
            *this = Number(stold(repr));
        }
    }
}

Number Number::getAbsolute() const {
    if (multiplePrecision) {
        return Number(abs(quotient));
    }
    return Number(fabsl(fraction));
}

Float Number::getLog10() const {
    if (multiplePrecision) {
        mpf_t f;  // C interface
        mpf_init(f);
        mpf_set_q(f, quotient.get_mpq_t());
        long int exponent;
        Float d = mpf_get_d_2exp(&exponent, f);  // f == d * 2^exponent
        Float lgF = log10l(d) + exponent * log10l(2);
        mpf_clear(f);
        return lgF;
    }
    return log10l(fraction);
}

Float Number::getLogSumExp(const Number& n) const {
    assert(!multiplePrecision);
    if (fraction == NEGATIVE_INFINITY) {
        return n.fraction;
    }
    if (n.fraction == NEGATIVE_INFINITY) {
        return fraction;
    }
    Float m = std::max(fraction, n.fraction);
    return log10l(exp10l(fraction - m) + exp10l(n.fraction - m)) + m;  // base-10 Cudd_addLogSumExp
}

bool Number::operator==(const Number& n) const {
    if (multiplePrecision) {
        return quotient == n.quotient;
    }
    return fraction == n.fraction;
}

bool Number::operator!=(const Number& n) const {
    return !(*this == n);
}

bool Number::operator<(const Number& n) const {
    if (multiplePrecision) {
        return quotient < n.quotient;
    }
    return fraction < n.fraction;
}

bool Number::operator<=(const Number& n) const {
    return *this < n || *this == n;
}

bool Number::operator>(const Number& n) const {
    if (multiplePrecision) {
        return quotient > n.quotient;
    }
    return fraction > n.fraction;
}

bool Number::operator>=(const Number& n) const {
    return *this > n || *this == n;
}

Number Number::operator*(const Number& n) const {
    if (multiplePrecision) {
        return Number(quotient * n.quotient);
    }
    return Number(fraction * n.fraction);
}

Number& Number::operator*=(const Number& n) {
    *this = *this * n;
    return *this;
}

Number Number::operator+(const Number& n) const {
    if (multiplePrecision) {
        return Number(quotient + n.quotient);
    }
    return Number(fraction + n.fraction);
}

Number& Number::operator+=(const Number& n) {
    *this = *this + n;
    return *this;
}

Number Number::operator-(const Number& n) const {
    if (multiplePrecision) {
        return Number(quotient - n.quotient);
    }
    return Number(fraction - n.fraction);
}

/* global functions ========================================================= */

std::ostream& operator<<(std::ostream& stream, const Number& n) {
    if (multiplePrecision) {
        stream << n.quotient;
    } else {
        stream << n.fraction;
    }

    return stream;
}