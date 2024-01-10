#pragma once

#include "pbformula.hpp"

#include "../../libraries/roundingsat/src/Solver.hpp"

namespace rsSolver {
    enum class VarState {UNKNOWN, _TRUE, _FALSE, TRUE, FALSE, UNCERTAIN};  // _TRUE maybe true

    extern string getVarState(VarState varState); 

    extern Int  judgeVarStates(const vector<VarState> &varStates);
    extern void updateVarStates(vector<VarState> &varStates, const vector<int> &curModel);
    extern bool tryBackBoneSolve(const Pbf &pbf, const vector<Int> &backBone, const Int lit, vector<Int> &curModel);
    extern bool getBackBone(const Pbf &pbf, vector<Int> &backBone);

    extern bool tryVivifSolve();
    extern bool getVivif(const Pbf& pbf);
}
