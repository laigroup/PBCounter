/* inclusions *****************************************************************/
#include "../interface/main.hpp"

/* classes ********************************************************************/

/* class OptionDict ***********************************************************/

void OptionDict::printOptionalOptions() const {
    cout << " Optional options:\n";
    util::printHelpOption();
    util::printCnfFileOption();
    util::printWeightFormatOption();
    util::printClusteringHeuristicOption();
    util::printCnfVarOrderingHeuristicOption();
    util::printDdVarOrderingHeuristicOption();
    util::printRandomSeedOption();
    // util::printVerbosityLevelOption();
    util::printDiagramPackageOption();
    util::printMultiplePrecisionOption();
    util::printMaximumMemoryOption();
    util::printPreprocessorOption();
}

void OptionDict::printHelp() const {
    cout << options->help({
        REQUIRED_OPTION_GROUP,
        // OPTIONAL_OPTION_GROUP
    });
    printOptionalOptions();
}

void OptionDict::printWelcome() const {
    bool commented = !helpFlag;

    printThickLine(commented);

    printComment("PBCounter: Weighted Model Counting on Pseudo-Boolean Constraints (help: './PBCounter -h')", 0, 1, commented);

    const string& version = "1.0";
    const string& date = "2023/05/05";
    // printComment("Version " + version + ", released on " + date, 0, 1, commented);

    printThickLine(commented);
}

OptionDict::OptionDict(int argc, char* argv[]) {
    options = new cxxopts::Options("PBCounter", "");

    options->add_options(OPTIONAL_OPTION_GROUP)
                        (HELP_OPTION, "help")
                        (INPUT_FILE_OPTION, "", cxxopts::value<string>()->default_value(STDIN_CONVENTION))
                        (WEIGHT_FORMAT_OPTION, "", cxxopts::value<string>()->default_value(to_string(DEFAULT_WEIGHT_FORMAT_CHOICE)))
                        (CLUSTERING_HEURISTIC_OPTION, "", cxxopts::value<string>()->default_value(to_string(DEFAULT_CLUSTERING_HEURISTIC_CHOICE)))
                        (CLUSTER_VAR_ORDER_OPTION, "", cxxopts::value<string>()->default_value(to_string(DEFAULT_CNF_VAR_ORDERING_HEURISTIC_CHOICE)))
                        (DIAGRAM_VAR_ORDER_OPTION, "", cxxopts::value<string>()->default_value(to_string(DEFAULT_DD_VAR_ORDERING_HEURISTIC_CHOICE)))
                        (RANDOM_SEED_OPTION, "", cxxopts::value<string>()->default_value(to_string(DEFAULT_RANDOM_SEED)))
                        (VERBOSITY_LEVEL_OPTION, "", cxxopts::value<string>()->default_value(to_string(DEFAULT_VERBOSITY_LEVEL_CHOICE)))
                        (DIAGRAM_PACKAGE_OPTION, "", cxxopts::value<string>()->default_value(DEFAULT_DIAGRAM_PACKAGE))
                        (MULTIPLE_PRECISION_OPTION, "", cxxopts::value<string>()->default_value(to_string(DEFAULT_MULTIPLE_PRECISION)))
                        (MAXIMUM_MEMORY_OPTION, "", cxxopts::value<string>()->default_value(to_string(DEFAULT_MAXIMUM_MEMORY)))
                        (PREPROCESSOR_OPTION, "", cxxopts::value<string>()->default_value(to_string(DEFAULT_PREPROCESSOR)));

    cxxopts::ParseResult result = options->parse(argc, argv);

    helpFlag = result["h"].as<bool>();

    printWelcome();

    inputFilePath = result[INPUT_FILE_OPTION].as<string>();

    weightFormatOption = std::stoll(result[WEIGHT_FORMAT_OPTION].as<string>());
    clusteringHeuristicOption = std::stoll(result[CLUSTERING_HEURISTIC_OPTION].as<string>());
    cnfVarOrderingHeuristicOption = std::stoll(result[CLUSTER_VAR_ORDER_OPTION].as<string>());
    ddVarOrderingHeuristicOption = std::stoll(result[DIAGRAM_VAR_ORDER_OPTION].as<string>());
    randomSeedOption = std::stoll(result[RANDOM_SEED_OPTION].as<string>());
    verbosityLevelOption = std::stoll(result[VERBOSITY_LEVEL_OPTION].as<string>());
    diagramPackageOption = result[DIAGRAM_PACKAGE_OPTION].as<string>();
    multiplePrecisionOption = std::stoll(result[MULTIPLE_PRECISION_OPTION].as<string>());
    maximumMemoryOtion = std::stoll(result[MAXIMUM_MEMORY_OPTION].as<string>());
    preprocessorOption = std::stoll(result[PREPROCESSOR_OPTION].as<string>());
}

/* namespaces *****************************************************************/

/* namespace testing **********************************************************/

void testing::test() {
    string filePath = "./examples/pb1.pbf";
    startTime = util::getTimePoint();
    Pbf pbf(filePath, WeightFormat::UNWEIGHTED);

    VarOrderingHeuristic ddVarOrderingHeuristic = VAR_ORDERING_HEURISTIC_CHOICES.at(DEFAULT_DD_VAR_ORDERING_HEURISTIC_CHOICE);
    VarOrderingHeuristic cnfVarOrderingHeuristic = VAR_ORDERING_HEURISTIC_CHOICES.at(DEFAULT_CNF_VAR_ORDERING_HEURISTIC_CHOICE);

    // MonolithicCounter monolithicCounter(ddVarOrderingHeuristic, false);
    // LinearCounter linearCounter(ddVarOrderingHeuristic, false);
    // BucketCounter bucketListCounter(false, cnfVarOrderingHeuristic, false, ddVarOrderingHeuristic, false);
    // BucketCounter bucketTreeCounter(true, cnfVarOrderingHeuristic, false, ddVarOrderingHeuristic, false);
    BouquetCounter bouquetListCounter(false, cnfVarOrderingHeuristic, false, ddVarOrderingHeuristic, false);
    BouquetCounter bouquetTreeCounter(true, cnfVarOrderingHeuristic, false, ddVarOrderingHeuristic, false);

    cout << "After build counter" << std::endl << std::endl;

    // Float m = monolithicCounter.getModelCount(pbf);
    // cout << std::left << std::setw(30) << "monolithic model count" << m << "\n";

    // Float l = linearCounter.getModelCount(pbf);
    // cout << std::left << std::setw(30) << "linear model count" << l << "\n";

    TimePoint lstTime = util::getTimePoint();

    // Float bel = bucketListCounter.getModelCount(pbf);
    // cout << std::left << std::setw(30) << "bucket list model count" << bel << " use time " << util::getSeconds(lstTime) << "\n";
    // lstTime = util::getTimePoint();

    // Float bet = bucketTreeCounter.getModelCount(pbf);
    // cout << std::left << std::setw(30) << "bucket tree model count" << bet << " use time " << util::getSeconds(lstTime) << "\n";
    // lstTime = util::getTimePoint();

    cout << std::fixed;
    cout << std::setprecision(1);

    Number bml = bouquetListCounter.getModelCount(pbf);
    cout << std::left << std::setw(30) << "bouquet list model count" << bml << " use time " << util::getSeconds(lstTime) << std::endl;
    lstTime = util::getTimePoint();

    Number bmt = bouquetTreeCounter.getModelCount(pbf);
    cout << std::left << std::setw(30) << "bouquet tree model count" << bmt << " use time " << util::getSeconds(lstTime) << std::endl;
    lstTime = util::getTimePoint();
}

/* namespace solving **********************************************************/

void solving::solveFile(
    const string& cnfFilePath,
    WeightFormat weightFormat,
    ClusteringHeuristic clusteringHeuristic,
    VarOrderingHeuristic cnfVarOrderingHeuristic,
    bool inverseCnfVarOrdering,
    VarOrderingHeuristic ddVarOrderingHeuristic,
    bool inverseDdVarOrdering) {
    if (verbosityLevel >= 1) {
        printComment("Reading command-line options...", 1);

        /* required: */
        util::printRow("cnfFilePath", cnfFilePath);

        /* optional: */
        util::printRow("weightFormat", util::getWeightFormatName(weightFormat));
        util::printRow("clustering", util::getClusteringHeuristicName(clusteringHeuristic));
        util::printRow("clusterVarOrder", util::getVarOrderingHeuristicName(cnfVarOrderingHeuristic));
        util::printRow("inverseClusterVarOrder", inverseCnfVarOrdering);
        util::printRow("diagramVarOrder", util::getVarOrderingHeuristicName(ddVarOrderingHeuristic));
        util::printRow("inverseDiagramVarOrder", inverseDdVarOrdering);
        util::printRow("randomSeed", randomSeed);
        util::printRow("diagramPackage", ddPackage);
        util::printRow("multiplePrecision", multiplePrecision);
    }

    if (ddPackage == SYLVAN_PACKAGE) {  // initializes Sylvan
        lace_init(1ll, 0);
        lace_startup(0, NULL, NULL);
        sylvan::sylvan_set_limits(maxMem * MEGA, tableRatio, initRatio);
        sylvan::sylvan_init_package();
        sylvan::sylvan_init_mtbdd();
        if (multiplePrecision) {
            sylvan::gmp_init();
        }
    }

    switch (clusteringHeuristic) {
        case ClusteringHeuristic::MONOLITHIC: {
            MonolithicCounter monolithicCounter(ddVarOrderingHeuristic, inverseDdVarOrdering);
            monolithicCounter.output(cnfFilePath, weightFormat);
            break;
        }
        case ClusteringHeuristic::LINEAR: {
            LinearCounter linearCounter(ddVarOrderingHeuristic, inverseDdVarOrdering);
            linearCounter.output(cnfFilePath, weightFormat);
            break;
        }
        case ClusteringHeuristic::BUCKET_LIST: {
            BucketCounter bucketCounter(false, cnfVarOrderingHeuristic, inverseCnfVarOrdering, ddVarOrderingHeuristic, inverseDdVarOrdering);
            bucketCounter.output(cnfFilePath, weightFormat);
            break;
        }
        case ClusteringHeuristic::BUCKET_TREE: {
            BucketCounter bucketCounter(true, cnfVarOrderingHeuristic, inverseCnfVarOrdering, ddVarOrderingHeuristic, inverseDdVarOrdering);
            bucketCounter.output(cnfFilePath, weightFormat);
            break;
        }
        case ClusteringHeuristic::BOUQUET_LIST: {
            BouquetCounter bouquetCounter(false, cnfVarOrderingHeuristic, inverseCnfVarOrdering, ddVarOrderingHeuristic, inverseDdVarOrdering);
            bouquetCounter.output(cnfFilePath, weightFormat);
            break;
        }
        case ClusteringHeuristic::BOUQUET_TREE: {
            BouquetCounter bouquetCounter(true, cnfVarOrderingHeuristic, inverseCnfVarOrdering, ddVarOrderingHeuristic, inverseDdVarOrdering);
            bouquetCounter.output(cnfFilePath, weightFormat);
            break;
        }
        default: {
            showError("no such clusteringHeuristic");
        }
    }

    if (ddPackage == SYLVAN_PACKAGE) {  // quits Sylvan
        sylvan::sylvan_quit();
        lace_exit();
    }
}

void solving::solveOptions(
    const string& cnfFilePath,
    Int weightFormatOption,
    Int clusteringHeuristicOption,
    Int cnfVarOrderingHeuristicOption,
    Int ddVarOrderingHeuristicOption) {
    WeightFormat weightFormat;
    try {
        weightFormat = WEIGHT_FORMAT_CHOICES.at(weightFormatOption);
    } catch (const std::out_of_range&) {
        showError("no such weightFormatOption: " + to_string(weightFormatOption));
    }

    ClusteringHeuristic clusteringHeuristic;
    try {
        clusteringHeuristic = CLUSTERING_HEURISTIC_CHOICES.at(clusteringHeuristicOption);
    } catch (const std::out_of_range&) {
        showError("no such clusteringHeuristicOption: " + to_string(clusteringHeuristicOption));
    }

    VarOrderingHeuristic cnfVarOrderingHeuristic;
    bool inverseCnfVarOrdering;
    try {
        cnfVarOrderingHeuristic = VAR_ORDERING_HEURISTIC_CHOICES.at(std::abs(cnfVarOrderingHeuristicOption));
        inverseCnfVarOrdering = cnfVarOrderingHeuristicOption < 0;
    } catch (const std::out_of_range&) {
        showError("no such cnfVarOrderingHeuristicOption: " + to_string(cnfVarOrderingHeuristicOption));
    }

    VarOrderingHeuristic ddVarOrderingHeuristic;
    bool inverseDdVarOrdering;
    try {
        ddVarOrderingHeuristic = VAR_ORDERING_HEURISTIC_CHOICES.at(std::abs(ddVarOrderingHeuristicOption));
        inverseDdVarOrdering = ddVarOrderingHeuristicOption < 0;
    } catch (const std::out_of_range&) {
        showError("no such ddVarOrderingHeuristicOption: " + to_string(ddVarOrderingHeuristicOption));
    }

    solveFile(
        cnfFilePath,
        weightFormat,
        clusteringHeuristic,
        cnfVarOrderingHeuristic,
        inverseCnfVarOrdering,
        ddVarOrderingHeuristic,
        inverseDdVarOrdering);
}

void solving::solveCommand(int argc, char* argv[]) {
    OptionDict optionDict(argc, argv);

    randomSeed = optionDict.randomSeedOption;                   // global variable
    verbosityLevel = optionDict.verbosityLevelOption;           // global variable
    startTime = util::getTimePoint();                           // global variable
    multiplePrecision = optionDict.multiplePrecisionOption;     // global variable
    ddPackage = optionDict.diagramPackageOption;                // golbal variable
    preprocessorFlag = optionDict.preprocessorOption;           // golbal variable

    if (optionDict.helpFlag) {
        optionDict.printHelp();
    } else {
        printComment("Process ID of this main program:", 1);
        printComment("pid " + to_string(getpid()));

        solveOptions(
            optionDict.inputFilePath,
            optionDict.weightFormatOption,
            optionDict.clusteringHeuristicOption,
            optionDict.cnfVarOrderingHeuristicOption,
            optionDict.ddVarOrderingHeuristicOption);
        cout << std::endl;

        util::printDuration(startTime);
    }
}

/* global functions ***********************************************************/

int main(int argc, char* argv[]) {
    cout << std::unitbuf;  // enables automatic flushing
    // testing::test();
    // Preprocessor::test(argc, argv);
    solving::solveCommand(argc, argv);
}