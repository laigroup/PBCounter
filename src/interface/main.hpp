#pragma once

/* inclusions *****************************************************************/

#include "../../libraries/cxxopts/include/cxxopts.hpp"

#include "counter.hpp"
#include "pbformula.hpp"
#include "visual.hpp"

/* classes ********************************************************************/

class OptionDict {
   public:
    /* optional: */
    bool helpFlag;
    string inputFilePath;
    Int weightFormatOption;
    Int clusteringHeuristicOption;
    Int cnfVarOrderingHeuristicOption;
    Int ddVarOrderingHeuristicOption;
    Int randomSeedOption;
    Int verbosityLevelOption;
    string diagramPackageOption;
    Int multiplePrecisionOption;
    Int maximumMemoryOtion;
    Int preprocessorOption;


    cxxopts::Options* options;

    void printOptionalOptions() const;
    void printHelp() const;
    void printWelcome() const;
    OptionDict(int argc, char* argv[]);
};

/* namespaces *****************************************************************/

namespace testing {
    void test();
}

namespace solving {
void solveFile(
    const string& inputFilePath,
    WeightFormat weightFormat,
    ClusteringHeuristic clusteringHeuristic,
    VarOrderingHeuristic cnfVarOrderingHeuristic,
    bool inverseCnfVarOrdering,
    VarOrderingHeuristic ddVarOrderingHeuristic,
    bool inverseDdVarOrdering);
void solveOptions(
    const string& cnfFilePath,
    Int weightFormatOption,
    Int clusteringHeuristicOption,
    Int cnfVarOrderingHeuristicOption,
    Int ddVarOrderingHeuristicOption);
void solveCommand(int argc, char* argv[]);
}  // namespace solving

/* global functions ***********************************************************/

int main(int argc, char* argv[]);