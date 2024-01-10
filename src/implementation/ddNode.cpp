#include "../interface/ddNode.hpp"

/* class Dd ================================================================= */

size_t Dd::maxDdLeafCount;
size_t Dd::maxDdNodeCount;

size_t Dd::prunedDdCount;
Float Dd::pruningDuration;

size_t Dd::getLeafCount() const {
    if (ddPackage == CUDD_PACKAGE) {
        return cuadd.CountLeaves();
    }
    MTBDD d = mtbdd.GetMTBDD();
    return mtbdd_leafcount(d);
}

size_t Dd::getNodeCount() const {
    if (ddPackage == CUDD_PACKAGE) {
        return cuadd.nodeCount();
    }
    return mtbdd.NodeCount();
}

Dd::Dd(const ADD& cuadd) {
    assert(ddPackage == CUDD_PACKAGE);
    this->cuadd = cuadd;
}

Dd::Dd(const Mtbdd& mtbdd) {
    assert(ddPackage == SYLVAN_PACKAGE);
    this->mtbdd = mtbdd;
}

Dd::Dd(const Dd& dd) {
    if (ddPackage == CUDD_PACKAGE) {
        *this = Dd(dd.cuadd);
    } else {
        *this = Dd(dd.mtbdd);
    }

    maxDdLeafCount = std::max(maxDdLeafCount, getLeafCount());
    maxDdNodeCount = std::max(maxDdNodeCount, getNodeCount());
}

Number Dd::extractConst() const {
    if (ddPackage == CUDD_PACKAGE) {
        ADD minTerminal = cuadd.FindMin();
        assert(minTerminal == cuadd.FindMax());
        return Number(cuddV(minTerminal.getNode()));
    }
    assert(mtbdd.isLeaf());
    if (multiplePrecision) {
        uint64_t val = mtbdd_getvalue(mtbdd.GetMTBDD());
        return Number(mpq_class(reinterpret_cast<mpq_ptr>(val)));
    }
    return Number(mtbdd_getdouble(mtbdd.GetMTBDD()));
}

Dd Dd::getConstDd(const Number& n, const Cudd& mgr) {
    // No logCounting
    if (ddPackage == CUDD_PACKAGE) {
        // return logCounting ? Dd(mgr->constant(n.getLog10())) : Dd(mgr->constant(n.fraction));
        return Dd(mgr.constant(n.fraction));
    }

    if (multiplePrecision) {
        mpq_t q;  // C interface
        mpq_init(q);
        mpq_set(q, n.quotient.get_mpq_t());
        Dd dd(Mtbdd(mtbdd_gmp(q)));
        mpq_clear(q);
        return dd;
    }
    return Dd(Mtbdd::doubleTerminal(n.fraction));
}

Dd Dd::getZeroDd(const Cudd& mgr) {
    return getConstDd(Number(), mgr);
}

Dd Dd::getOneDd(const Cudd& mgr) {
    return getConstDd(Number("1"), mgr);
}

Dd Dd::getConstZeroDd() {
    assert(ddPackage == SYLVAN_PACKAGE);
    return sylvan::Mtbdd::mtbddOne();
}

Dd Dd::getConstOneDd() {
    assert(ddPackage == SYLVAN_PACKAGE);
    return sylvan::Mtbdd::mtbddZero();
}

Dd Dd::getVarDd(Int ddVar, const Cudd& mgr) {
    if (ddPackage == CUDD_PACKAGE) {
        return Dd(mgr.addVar(ddVar));
    }
    return Dd(Mtbdd::mtbddVar(ddVar));
}

Dd Dd::getVarDd(Int ddVar, bool val, const Cudd& mgr) {
    if (ddPackage == CUDD_PACKAGE) {
        // if (logCounting) {
        //     return Dd(mgr->addLogVar(ddVar, val));
        // }
        ADD d = mgr.addVar(ddVar);
        return val ? Dd(d) : Dd(d.Cmpl());
    }
    MTBDD d0 = getZeroDd(mgr).mtbdd.GetMTBDD();
    MTBDD d1 = getOneDd(mgr).mtbdd.GetMTBDD();
    return val ? Dd(mtbdd_makenode(ddVar, d0, d1)) : Dd(mtbdd_makenode(ddVar, d1, d0));
}

bool Dd::operator!=(const Dd& rightDd) const {
    if (ddPackage == CUDD_PACKAGE) {
        return cuadd != rightDd.cuadd;
    }
    return mtbdd != rightDd.mtbdd;
}

Dd Dd::getComposition(Int ddVar, bool val, const Cudd& mgr) const {
    if (ddPackage == CUDD_PACKAGE) {
        if (util::isFound(ddVar, cuadd.SupportIndices())) {
            return Dd(cuadd.Compose(val ? mgr.addOne() : mgr.addZero(), ddVar));
        }
        return *this;
    }
    sylvan::MtbddMap m;
    m.put(ddVar, val ? Mtbdd::mtbddOne() : Mtbdd::mtbddZero());
    return Dd(mtbdd.Compose(m));
}

Dd Dd::getProduct(const Dd& dd) const {
    if (ddPackage == CUDD_PACKAGE) {
        // return logCounting ? Dd(cuadd + dd.cuadd) : Dd(cuadd * dd.cuadd);
        return Dd(cuadd * dd.cuadd);
    }
    if (multiplePrecision) {
        LACE_ME;
        return Dd(Mtbdd(gmp_times(mtbdd.GetMTBDD(), dd.mtbdd.GetMTBDD())));
    }
    return Dd(mtbdd * dd.mtbdd);
}

Dd Dd::getSum(const Dd& dd) const {
    if (ddPackage == CUDD_PACKAGE) {
        // return logCounting ? Dd(cuadd.LogSumExp(dd.cuadd)) : Dd(cuadd + dd.cuadd);
        return Dd(cuadd + dd.cuadd);
    }
    if (multiplePrecision) {
        LACE_ME;
        return Dd(Mtbdd(gmp_plus(mtbdd.GetMTBDD(), dd.mtbdd.GetMTBDD())));
    }
    return Dd(mtbdd + dd.mtbdd);
}

Dd Dd::getMax(const Dd& dd) const {
    if (ddPackage == CUDD_PACKAGE) {
        return Dd(cuadd.Maximum(dd.cuadd));
    }
    if (multiplePrecision) {
        LACE_ME;
        return Dd(Mtbdd(gmp_max(mtbdd.GetMTBDD(), dd.mtbdd.GetMTBDD())));
    }
    return Dd(mtbdd.Max(dd.mtbdd));
}

Dd Dd::getXor(const Dd& dd) const {
    assert(ddPackage == CUDD_PACKAGE);
    // return logCounting ? Dd(cuadd.LogXor(dd.cuadd)) : Dd(cuadd.Xor(dd.cuadd));
    return Dd(cuadd.Xor(dd.cuadd));
}

Dd Dd::getIte(Dd& tdd, Dd& fdd) const {
    if (ddPackage == CUDD_PACKAGE) {
        return Dd(cuadd.Ite(tdd.cuadd, fdd.cuadd));
    }
    return Dd(mtbdd.Ite(tdd.mtbdd, fdd.mtbdd));
}

Set<Int> Dd::getSupport() const {
    Set<Int> support;
    if (ddPackage == CUDD_PACKAGE) {
        for (Int ddVar : cuadd.SupportIndices()) {
            support.insert(ddVar);
        }
    } else {
        Mtbdd cube = mtbdd.Support();  // conjunction of all vars appearing in mtbdd
        while (!cube.isOne()) {
            support.insert(cube.TopVar());
            cube = cube.Then();
        }
    }
    return support;
}

Dd Dd::getBoolDiff(const Dd& rightDd) const {
    assert(ddPackage == CUDD_PACKAGE);
    return Dd((cuadd - rightDd.cuadd).BddThreshold(0).Add());
}

bool Dd::evalAssignment(vector<int>& ddVarAssignment) const {
    assert(ddPackage == CUDD_PACKAGE);
    Number n = Dd(cuadd.Eval(&ddVarAssignment.front())).extractConst();
    return n == Number(1);
}

Int Dd::getTopVar() const {
    assert(ddPackage == SYLVAN_PACKAGE);
    return mtbdd.TopVar();
}

// follows the high edge (then) of the node of this Dd
Dd Dd::getThen() const {
    assert(ddPackage == SYLVAN_PACKAGE);
    return Dd(mtbdd.Then());
}

// follows the low edge (else) of the node of this Dd
Dd Dd::getElse() const {
    assert(ddPackage == SYLVAN_PACKAGE);
    return Dd(mtbdd.Else());
}

bool Dd::isConstOne() const {
    assert(ddPackage == SYLVAN_PACKAGE);
    return mtbdd.isOne();
}

bool Dd::isConstZero() const {
    assert(ddPackage == SYLVAN_PACKAGE);
    return mtbdd.isZero();
}

bool Dd::isLeaf() const {
    assert(ddPackage == SYLVAN_PACKAGE);
    return mtbdd.isLeaf();
}

string Dd::leafStr() const {
    assert(ddPackage == SYLVAN_PACKAGE);
    char buf[64];

    return sylvan::mtbdd_leaf_to_str(mtbdd.GetMTBDD(), buf, 64);
}

void Dd::writeDotFile(const Cudd& mgr, const string& dotFileDir) const {
    string filePath = dotFileDir + "dd" + to_string(dotFileIndex++) + ".dot";
    FILE* file = fopen(filePath.c_str(), "wb");  // writes to binary file

    if (ddPackage == CUDD_PACKAGE) {  // davidkebo.com/cudd#cudd6
        DdNode** ddNodeArray = static_cast<DdNode**>(malloc(sizeof(DdNode*)));
        ddNodeArray[0] = cuadd.getNode();
        Cudd_DumpDot(mgr.getManager(), 1, ddNodeArray, NULL, NULL, file);
        free(ddNodeArray);
    } else {
        mtbdd_fprintdot_nc(file, mtbdd.GetMTBDD());
    }

    fclose(file);
    cout << "c wrote decision diagram to file " << filePath << "\n";
}
