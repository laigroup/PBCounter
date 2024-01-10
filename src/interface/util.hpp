/* DAG of inclusions:
.>>>> graph  >> formula
^               v
util >> join >> counter >> main*
v               ^
.>>>>>>>>>>>>>> visual
*/

#pragma once

/* inclusions *****************************************************************/

#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <assert.h>

#include <gmp.h>
#include <gmpxx.h> 

#include "../../libraries/cudd/cplusplus/cuddObj.hh"
#include "../../libraries/cudd/cudd/cuddInt.h"

#include "../../libraries/sylvan/src/sylvan_gmp.h"
#include "../../libraries/sylvan/src/sylvan_obj.hpp"

/* uses ***********************************************************************/

using std::cout;
using std::endl;
using std::string;
using std::to_string;
using std::vector;
// using std::max;

/* types **********************************************************************/

using Float = double;      // std::stod // OPTIL would complain about 'long double'
using Int = int_fast64_t;  // std::stoll
using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

template <typename K, typename V>
using Map = std::unordered_map<K, V>;
template <typename K, typename V, typename H, typename E>
using Mymap = std::unordered_map<K, V, H, E>;
template <typename T>
using Set = std::unordered_set<T>;
template <typename T1, typename T2>
using Pair = std::pair<T1, T2>;

// /* global variables ***********************************************************/

extern Int randomSeed;  // for reproducibility
extern Int verbosityLevel;
extern TimePoint startTime;
extern bool multiplePrecision;
extern string ddPackage;
extern Float memSensitivity;
extern Int dotFileIndex;  // dotFileIndex 输出 dot 文件 write 后会自加
extern Int maxMem;
extern Float tableRatio;
extern Float initRatio;
extern bool preprocessorFlag;

// /* constants ******************************************************************/
extern const Float MEGA;

extern const string& COMMENT_WORD;
extern const string& PROBLEM_WORD;
extern const char& VARIABLE_WORD;

extern const string& COMMENT_VARIABLE_WORD;
extern const string& COMMENT_CONSTRAINT_WORD;

extern const string& EQUAL_WORD;
extern const string& GEQUAL_WORD;
extern const string& LEQUAL_WORD;

extern const string& END_LINE_WORD;

extern const string& STDIN_CONVENTION;

extern const string& REQUIRED_OPTION_GROUP;
extern const string& OPTIONAL_OPTION_GROUP;

extern const string& HELP_OPTION;
extern const string& INPUT_FILE_OPTION;
extern const string& WEIGHT_FORMAT_OPTION;
extern const string& CLUSTERING_HEURISTIC_OPTION;
extern const string& CLUSTER_VAR_ORDER_OPTION;
extern const string& DIAGRAM_VAR_ORDER_OPTION;
extern const string& RANDOM_SEED_OPTION;
extern const string& VERBOSITY_LEVEL_OPTION;
extern const string& DIAGRAM_PACKAGE_OPTION;
extern const string& MULTIPLE_PRECISION_OPTION;
extern const string& MAXIMUM_MEMORY_OPTION;
extern const string& PREPROCESSOR_OPTION;

extern const string& CUDD_PACKAGE;
extern const string& SYLVAN_PACKAGE;

extern const string DEFAULT_DIAGRAM_PACKAGE;
extern const Int DEFAULT_MULTIPLE_PRECISION;
extern const Int DEFAULT_MAXIMUM_MEMORY;
extern const Int DEFAULT_PREPROCESSOR;

enum class WeightFormat { UNWEIGHTED,
                          WEIGHTED };
extern const std::map<Int, WeightFormat> WEIGHT_FORMAT_CHOICES;
extern const Int DEFAULT_WEIGHT_FORMAT_CHOICE;

enum class ClusteringHeuristic { MONOLITHIC,
                                 LINEAR,
                                 BUCKET_LIST,
                                 BUCKET_TREE,
                                 BOUQUET_LIST,
                                 BOUQUET_TREE };
extern const std::map<Int, ClusteringHeuristic> CLUSTERING_HEURISTIC_CHOICES;
extern const Int DEFAULT_CLUSTERING_HEURISTIC_CHOICE;

enum class VarOrderingHeuristic {
    DUMMY_VAR_ORDERING_HEURISTIC,  // would trigger error in Cnf::getVarOrdering
    APPEARANCE,
    DECLARATION,
    RANDOM,
    MCS,
    LEXP,
    LEXM
};
extern const std::map<Int, VarOrderingHeuristic> VAR_ORDERING_HEURISTIC_CHOICES;
extern const Int DEFAULT_CNF_VAR_ORDERING_HEURISTIC_CHOICE;
extern const Int DEFAULT_DD_VAR_ORDERING_HEURISTIC_CHOICE;

extern const Int DEFAULT_RANDOM_SEED;

extern const vector<Int> VERBOSITY_LEVEL_CHOICES;
extern const Int DEFAULT_VERBOSITY_LEVEL_CHOICE;

extern const Float NEGATIVE_INFINITY;

extern const Int DUMMY_MIN_INT;
extern const Int DUMMY_MAX_INT;

extern const string& DUMMY_STR;

extern const string& DOT_DIR;

class Number {
   public:
    mpq_class quotient;
    Float fraction;

    Number(const mpq_class& q);  // multiplePrecision
    Number(Float f);             // !multiplePrecision
    Number(const Number& n);
    Number(const string& repr = "0");  // `repr` is `<int>/<int>` or `<float>`

    Number getAbsolute() const;
    Float getLog10() const;
    Float getLogSumExp(const Number& n) const;
    bool operator==(const Number& n) const;
    bool operator!=(const Number& n) const;
    bool operator<(const Number& n) const;
    bool operator<=(const Number& n) const;
    bool operator>(const Number& n) const;
    bool operator>=(const Number& n) const;
    Number operator*(const Number& n) const;
    Number& operator*=(const Number& n);
    Number operator+(const Number& n) const;
    Number& operator+=(const Number& n);
    Number operator-(const Number& n) const;
};


/* namespaces *****************************************************************/

namespace util {
    bool isInt(Float d);

    /* functions: printing ******************************************************/

    void printComment(const string& message, Int preceedingNewLines = 0, Int followingNewLines = 1, bool commented = true);
    void printSolutionLine(Number modelCount, WeightFormat weightFormat, Int preceedingThinLines = 1, Int followingThinLines = 1);
    void printCnfSolutionLine(WeightFormat weightFormat, Number modelCount, Int preceedingThinLines = 1, Int followingThinLines = 1);

    void printBoldLine(bool commented);
    void printThickLine(bool commented = true);
    void printThinLine();

    void printHelpOption();
    void printCnfFileOption();
    void printWeightFormatOption();
    void printClusteringHeuristicOption();
    void printCnfVarOrderingHeuristicOption();
    void printDdVarOrderingHeuristicOption();
    void printRandomSeedOption();
    void printVerbosityLevelOption();
    void printDiagramPackageOption();
    void printMultiplePrecisionOption();
    void printMaximumMemoryOption();
    void printPreprocessorOption();

    /* functions: argument parsing **********************************************/

    vector<string> getArgV(int argc, char* argv[]);

    string getWeightFormatName(WeightFormat weightFormat);
    string getClusteringHeuristicName(ClusteringHeuristic clusteringHeuristic);
    string getVarOrderingHeuristicName(VarOrderingHeuristic varOrderingHeuristic);
    string getVerbosityLevelName(Int verbosityLevel);

    /* functions: CNF ***********************************************************/

    Int getCnfVar(Int literal);
    Set<Int> getClauseCnfVars(const vector<Int>& clause);
    Set<Int> getClusterCnfVars(const vector<Int>& cluster, const vector<vector<Int>>& clauses);

    bool appearsIn(Int cnfVar, const vector<Int>& clause);
    bool isPositiveLiteral(Int literal);

    Int getLiteralRank(Int literal, const vector<Int>& cnfVarOrdering);
    Int getMinClauseRank(const vector<Int>& clause, const vector<Int>& cnfVarOrdering);
    Int getMaxClauseRank(const vector<Int>& clause, const vector<Int>& cnfVarOrdering);

    void printClause(const vector<Int>& clause);
    void printCnf(const vector<vector<Int>>& clauses);
    void printLiteralWeights(const Map<Int, Float>& literalWeights);

    /* functions: PBF ***********************************************************/
    Int getPbfVar(Int literal);
    // Set<Int> getVlausePbfVars(const vector<Int> &clause);
    // Set<Int> getClusterPbfVars(const vector<Int> &cluster, const vector<vector<Int>> &clauses);

    void formatConstraint(vector<Int>& clause, vector<Int>& coefficient, Int& limit);
    void inverseConstraint(vector<Int>& clause, vector<Int>& coefficient, Int& limit);

    void printConstraint(const vector<Int>& clause, const vector<Int>& coefficent, const string& option, const Int& limit);
    void printPbf(const vector<vector<Int>>& clauses, const vector<vector<Int>>& coefficents, const vector<string> &options, const vector<Int>& limits);

    /* functions: timing ********************************************************/

    TimePoint getTimePoint();
    Float getSeconds(TimePoint startTime);
    void printDuration(TimePoint startTime);

    /* functions: error handling ************************************************/

    void showWarning(const string& message, bool commented = true);
    void showError(const string& message, bool commented = true);

    /* functions: templates implemented in headers to avoid linker errors *******/

    template <typename T>
    void printRow(const string& name, const T& value) {
        cout << COMMENT_WORD << " " << std::left << std::setw(30) << name;
        cout << std::left << std::setw(15) << value << "\n";
    }

    template <typename T>
    void printContainer(const T& container) {
        cout << "printContainer:\n";
        for (const auto& member : container) {
            cout << "\t" << member << "\t";
        }
        cout << "\n";
    }

    template <typename K, typename V>
    void printMap(const Map<K, V>& m) {
        cout << "printMap:\n";
        for (const auto& kv : m) {
            cout << "\t" << kv.first << "\t:\t" << kv.second << "\n";
        }
        cout << "\n";
    }

    template <typename Key, typename Value>
    bool isLessValued(std::pair<Key, Value> a, std::pair<Key, Value> b) {
        return a.second < b.second;
    }

    template <typename T>
    T getSoleMember(const vector<T>& v) {
        if (v.size() != 1)
            showError("vector is not singleton");
        return v.front();
    }

    template <typename T>
    void popBack(T& element, vector<T>& v) {
        if (v.empty())
            showError("vector is empty");
        element = v.back();
        v.pop_back();
    }

    template <typename T>
    void invert(T& t) {
        std::reverse(t.begin(), t.end());
    }

    template <typename T, typename U>
    bool isFound(const T& element, const U& container) {
        return std::find(std::begin(container), std::end(container), element) != std::end(container);
    }

    template <typename T, typename U1, typename U2>
    void differ(Set<T>& diff, const U1& members, const U2& nonmembers) {
        for (const auto& member : members) {
            if (!isFound(member, nonmembers)) {
                diff.insert(member);
            }
        }
    }

    template <typename T, typename U>
    void unionize(Set<T>& unionSet, const U& container) {
        for (const auto& member : container)
            unionSet.insert(member);
    }

    template <typename T, typename U>
    bool isDisjoint(const T& container, const U& container2) {
        for (const auto& member : container) {
            for (const auto& member2 : container2) {
                if (member == member2) {
                    return false;
                }
            }
        }
        return true;
    }

    template <typename T>
    Number adjustModelCount(Number apparentModelCount, const T& projectedCnfVars, const Map<Int, Number>& literalWeights) {
        Number totalModelCount = apparentModelCount;

        Int totalLiteralCount = literalWeights.size();
        if (totalLiteralCount % 2 == 1)
            showError("odd total literal count");

        Int totalVarCount = totalLiteralCount / 2;
        if (totalVarCount < projectedCnfVars.size())
            showError("more projected vars than total vars");

        for (Int cnfVar = 1; cnfVar <= totalVarCount; cnfVar++) {
            if (!isFound(cnfVar, projectedCnfVars)) {
                totalModelCount *= literalWeights.at(cnfVar) + literalWeights.at(-cnfVar);
            }
        }

        if (totalModelCount == Number()) {
            showWarning("floating-point underflow may have occured");
        }
        return totalModelCount;
    }

    template <typename T>
    void shuffleRandomly(T& container) {
        std::mt19937 generator;
        generator.seed(randomSeed);
        std::shuffle(container.begin(), container.end(), generator);
    }

    template <typename Dd>
    Set<Int> getSupportSuperset(const vector<Dd>& dds) {
        Set<Int> supersupport;
        for (const Dd& dd : dds)
            for (Int var : dd.getSupport())
                supersupport.insert(var);
        return supersupport;
    }

    template <typename Dd>
    Int getMinDdRank(const Dd& dd, const vector<Int>& ddVarToCnfVarMap, const vector<Int>& cnfVarOrdering) {
        Int minRank = DUMMY_MAX_INT;
        // for (Int ddVar : getSupport(dd)) {
        for (Int ddVar : dd.getSupport()) {
            Int cnfVar = ddVarToCnfVarMap.at(ddVar);
            Int rank = getLiteralRank(cnfVar, cnfVarOrdering);
            if (rank < minRank)
                minRank = rank;
        }
        return minRank;
    }

    template <typename Dd>
    Int getMaxDdRank(const Dd& dd, const vector<Int>& ddVarToCnfVarMap, const vector<Int>& cnfVarOrdering) {
        Int maxRank = DUMMY_MIN_INT;
        // for (Int ddVar : getSupport(dd)) {
        for (Int ddVar : dd.getSupport()) {
            Int cnfVar = ddVarToCnfVarMap.at(ddVar);
            Int rank = getLiteralRank(cnfVar, cnfVarOrdering);
            if (rank > maxRank)
                maxRank = rank;
        }
        return maxRank;
    }
}  // namespace util

/* classes ********************************************************************/

class MyError {
   public:
    MyError(const string& message, bool commented);
};

/* global functions ========================================================= */

std::ostream& operator<<(std::ostream& stream, const Number& n);