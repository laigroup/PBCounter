#include "../interface/rsInterface.hpp"

namespace rs {
bool asynch_interrupt;
Options options;
Stats stats;
}


namespace rsSolver {

extern string getVarState(VarState varState) {
    switch(varState) {
        case VarState::UNKNOWN: return "UNKNOWN";
        case VarState::_TRUE:   return "_TRUE";
        case VarState::_FALSE:  return "_FALSE";
        case VarState::TRUE:    return "TRUE";
        case VarState::FALSE:   return "FALSE";
        case VarState::UNCERTAIN:   return "UNCERTAIN";
    }
    return "NONE";
}

extern Int judgeVarStates(const vector<VarState> &varStates) {
    for(Int i = 1; i < varStates.size(); i++) {
        if(varStates.at(i) == VarState::_TRUE) return i;
        else if(varStates.at(i) == VarState::_FALSE) return -i;
    }
    return 0;
}

extern void updateVarStates(vector<VarState> &varStates, const vector<int> &curModel) {
    assert(varStates.size() == curModel.size());
    for(Int i = 1; i < varStates.size(); i++) {
        if(curModel.at(i) > 0) {
            if(varStates.at(i) == VarState::UNKNOWN) varStates.at(i) = VarState::_TRUE;
            else if(varStates.at(i) == VarState::_FALSE) varStates.at(i) = VarState::UNCERTAIN;
            else if(varStates.at(i) == VarState::FALSE)  showError("Conflict in updateVarStates");
        } else if(curModel.at(i) < 0) {
            if(varStates.at(i) == VarState::UNKNOWN) varStates.at(i) = VarState::_FALSE;
            else if(varStates.at(i) == VarState::_TRUE) varStates.at(i) = VarState::UNCERTAIN;
            else if(varStates.at(i) == VarState::TRUE)  showError("Conflict in updateVarStates");
        }
        // cout << i << ": " << getVarState(varStates.at(i)) << "  " << curModel.at(i) << std::endl;
    }
}

extern bool tryBackBoneSolve(const Pbf &pbf, const vector<Int> &backBone, const Int lit, vector<int> &curModel) {
    rs::Solver solver;
    solver.init();                                  // call after having read options

    rs::CeArb input = solver.cePools.takeArb();
    // rs::CeArb objective = solver.cePools.takeArb();
    const vector<vector<Int> > &clasues = pbf.getClauses();
    const vector<vector<Int> > &coefs   = pbf.getCoefficients();
    const vector<string> &options = pbf.getOptions();
    const vector<Int>    &limits  = pbf.getLimits();

    // in pbf is <= K, but roundingsat need >= K 
    for(Int i = 0; i < clasues.size(); i++) {
        input->reset();
        for(Int j = 0; j < clasues.at(i).size(); j++) {
            solver.setNbVars(std::abs(clasues.at(i).at(j)), true);
            input->addLhs(-coefs.at(i).at(j), clasues.at(i).at(j));
        }
        input->addRhs(-limits.at(i));
        
        if (solver.addConstraint(input, rs::Origin::FORMULA).second == rs::ID_Unsat) {
            return false;
        }

        if(options.at(i) == EQUAL_WORD) {
            input->invert();
            if (solver.addConstraint(input, rs::Origin::FORMULA).second == rs::ID_Unsat) {
                return false;
            }
        }
    }

    // 1 * BackBone.at(i) = 1
    for(Int i = 0; i < backBone.size(); i++) {
        input->reset();
        solver.setNbVars(std::abs(backBone.at(i)), true);
        input->addLhs(1, backBone.at(i));
        input->addRhs(1);
        if (solver.addConstraint(input, rs::Origin::FORMULA).second == rs::ID_Unsat) {
            return false;
        }
        input->invert();
        if (solver.addConstraint(input, rs::Origin::FORMULA).second == rs::ID_Unsat) {
            return false;
        }
    }

    // 1 * lit = 1
    if (lit != 0) {
        input->reset();
        solver.setNbVars(std::abs(lit), true);
        input->addLhs(1, lit);
        input->addRhs(1);
        if (solver.addConstraint(input, rs::Origin::FORMULA).second == rs::ID_Unsat) {
            return false;
        }
        input->invert();
        if (solver.addConstraint(input, rs::Origin::FORMULA).second == rs::ID_Unsat) {
            return false;
        }
    }
    
    while (true) {
        rs::SolveState reply = rs::aux::timeCall<rs::SolveState>([&] { return solver.solve().state; }, rs::stats.SOLVETIME);
        // assert(reply != SolveState::INCONSISTENT);
        if (reply == rs::SolveState::SAT) {
            // cout << "SAT\n";
            curModel = solver.lastSol;
            // rs::quit::printSol(solver.lastSol);
            return true;
        } else if (reply == rs::SolveState::UNSAT) {
            // cout << "UNSAT\n";
            return false;
        }
    } 
}

extern bool getBackBone(const Pbf &pbf, vector<Int> &backBone) {
    char *op[] = {"options", "--verbosity=0"};
    rs::options.parseCommandLine(2, op);

    vector<int> curModel;
    vector<VarState> varStates;
    Int varCnt = pbf.getApparentVarCount();

    varStates.resize(varCnt + 1, VarState::UNKNOWN);
    backBone.clear();

    if(tryBackBoneSolve(pbf, backBone, 0, curModel)) {
        updateVarStates(varStates, curModel);
    } else return false;

    Int lit;
    while((lit = judgeVarStates(varStates)) != 0) {
        // cout << std::endl << std::endl << "GetLit = " << lit << std::endl;
        if(tryBackBoneSolve(pbf, backBone, -lit, curModel)) {
            if(varStates.at(std::abs(lit)) == VarState::UNKNOWN) {
                varStates.at(std::abs(lit)) = lit > 0 ? VarState::_FALSE : VarState::_TRUE;
            } else varStates.at(std::abs(lit)) = VarState::UNCERTAIN;

            updateVarStates(varStates, curModel);
        } else {
            if(varStates.at(std::abs(lit)) == VarState::UNKNOWN) {
                varStates.at(std::abs(lit)) = lit > 0 ? VarState::_TRUE : VarState::_FALSE;
            } else if(varStates.at(std::abs(lit)) == VarState::_FALSE) {
                assert(lit < 0);
                varStates.at(std::abs(lit)) = VarState::FALSE;
                backBone.push_back(lit);
            } else if(varStates.at(std::abs(lit)) == VarState::_TRUE) {
                assert(lit > 0);
                varStates.at(std::abs(lit)) = VarState::TRUE;
                backBone.push_back(lit);
            }
        }
        // cout << "BackBone size: " << backBone.size() << std::endl;
        // for(auto p : backBone) cout << p << " ";
        // cout << std::endl;
    }
    // trySolve(pbf, backBone, curModel);
    return true;    
}


} 

