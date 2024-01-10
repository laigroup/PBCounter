#pragma once

#include "util.hpp"

/* uses ===================================================================== */

using sylvan::gmp_op_max_CALL;
using sylvan::gmp_op_plus_CALL;
using sylvan::gmp_op_times_CALL;
using sylvan::Mtbdd;
using sylvan::MTBDD;
using sylvan::mtbdd_apply_CALL;
using sylvan::mtbdd_fprintdot_nc;
using sylvan::mtbdd_getdouble;
using sylvan::mtbdd_getvalue;
using sylvan::mtbdd_gmp;
using sylvan::mtbdd_leafcount_more;
using sylvan::mtbdd_makenode;

using util::printRow;

// 分别计算出内置类型的 Hash Value 然后对它们进行 Combine 得到一个哈希值
// 一般直接采用移位加异或（XOR）得到哈希值
struct HashFunc {
    template <typename T, typename U>
    size_t operator()(const std::pair<T, U>& p) const {
        return std::hash<T>()(p.first) ^ std::hash<U>()(p.second);
    }
};

// 键值比较，哈希碰撞的比较定义，需要直到两个自定义对象是否相等
struct EqualKey {
    template <typename T, typename U>
    bool operator()(const std::pair<T, U>& p1, const std::pair<T, U>& p2) const {
        return p1.first == p2.first && p1.second == p2.second;
    }
};

/* class Dd ================================================================= */
class Dd {  // wrapper for CUDD and Sylvan
   public:
    static size_t maxDdLeafCount;
    static size_t maxDdNodeCount;

    static size_t prunedDdCount;
    static Float pruningDuration;

    ADD cuadd;    // CUDD
    Mtbdd mtbdd;  // Sylvan

    size_t getLeafCount() const;
    size_t getNodeCount() const;

    Dd(const ADD& cuadd);
    Dd(const Mtbdd& mtbdd);
    Dd(const Dd& dd);

    Number extractConst() const;                             // does not read logCounting
    static Dd getConstDd(const Number& n, const Cudd& mgr);  // reads logCounting
    static Dd getZeroDd(const Cudd& mgr);                    // returns minus infinity if logCounting
    static Dd getOneDd(const Cudd& mgr);                     // returns zero if logCounting
    static Dd getVarDd(Int ddVar, const Cudd& mgr);
    static Dd getVarDd(Int ddVar, bool val, const Cudd& mgr);
    // static const Cudd* newMgr(Float mem, Int threadIndex = 0);  // CUDD
    bool operator!=(const Dd& rightDd) const;
    // bool operator<(const Dd& rightDd) const;                        // *this < rightDd (top of priotity queue is rightmost element)
    Dd getComposition(Int ddVar, bool val, const Cudd& mgr) const;  // restricts *this to ddVar=val
    Dd getProduct(const Dd& dd) const;                              // reads logCounting
    Dd getSum(const Dd& dd) const;                                  // reads logCounting
    Dd getMax(const Dd& dd) const;                                  // real max (not 0-1 max)
    Dd getXor(const Dd& dd) const;                                  // must be 0-1 DDs
    Dd getIte(Dd& tdd, Dd& fdd) const;                      // if f then tdd else fdd
    Set<Int> getSupport() const;
    Dd getBoolDiff(const Dd& rightDd) const;  // returns 0-1 DD for *this >= rightDd
    bool evalAssignment(vector<int>& ddVarAssignment) const;
    // Dd getAbstraction(
    //     Int ddVar,
    //     const vector<Int>& ddVarToCnfVarMap,
    //     const Map<Int, Number>& literalWeights,
    //     const Assignment& assignment,
    //     bool additiveFlag,  // ? getSum : getMax
    //     vector<pair<Int, Dd>>& maximizationStack,
    //     const Cudd* mgr) const;
    // Dd getPrunedDd(Float lowerBound, const Cudd* mgr) const;

    Int getTopVar() const;
    Dd  getThen() const;
    Dd  getElse() const;

    static Dd getConstZeroDd();
    static Dd getConstOneDd();
    bool isConstOne() const;
    bool isConstZero() const;

    bool isLeaf() const;
    string leafStr() const;

    void writeDotFile(const Cudd& mgr, const string& dotFileDir = "./") const;
    // static void writeInfoFile(const Cudd* mgr, const string& filePath);
};