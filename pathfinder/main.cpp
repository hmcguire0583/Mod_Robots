#include <chrono>
#include <iostream>
#include <getopt.h>
#include <string>
#include "moves/MoveManager.h"
#include "search/ConfigurationSpace.h"
#include <boost/format.hpp>
#include <nlohmann/json.hpp>
#include "lattice/LatticeSetup.h"
#include "moves/Scenario.h"
#include "search/SearchAnalysis.h"
#include "search/HeuristicCache.h"

#ifndef GENERATE_FINAL_STATE
#define GENERATE_FINAL_STATE false
#endif

#ifndef PRINT_PATH
#define PRINT_PATH false
#endif

int main(int argc, char* argv[]) {
    bool ignoreColors = false;
    std::string initialFile;
    std::string finalFile;
    std::string exportFile;
    std::string analysisFile;
    std::string movesFolder;
    std::string searchMethod;
    std::string heuristic;
    std::string edgeCheck;

    // Define the long options
    static option long_options[] = {
        {"ignore-colors", no_argument, nullptr, 'i'},
        {"initial-file", required_argument, nullptr, 'I'},
        {"final-file", required_argument, nullptr, 'F'},
        {"export-file", required_argument, nullptr, 'e'},
        {"analysis-file", required_argument, nullptr, 'a'},
        {"moves-folder", required_argument, nullptr, 'm'},
        {"search-method", required_argument, nullptr, 's'},
        {"heuristic", required_argument, nullptr, 'h'},
        {"edge-check", required_argument, nullptr, 'c'},
        {nullptr, 0, nullptr, 0}
    };

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "iI:F:e:a:m:s:h:c:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'i':
                ignoreColors = true;
                break;
            case 'I':
                initialFile = optarg;
                break;
            case 'F':
                finalFile = optarg;
                break;
            case 'e':
                exportFile = optarg;
                break;
            case 'a':
                analysisFile = optarg;
                break;
            case 'm':
                movesFolder = optarg;
                break;
            case 's':
                searchMethod = optarg;
                break;
            case 'h':
                heuristic = optarg;
                break;
            case 'c':
                edgeCheck = optarg;
                break;
            case '?':
                break;
            default:
                abort();
        }
    }

    // Prompt user for names for initial and final state files if they are not given as command line arguments
    if (initialFile.empty()) {
        std::cout << "Path to initial state:" << std::endl;
        int numTries = 0;
        bool invalidPath = true;
        while (invalidPath) {
            std::cin >> initialFile;
            if (std::filesystem::exists(initialFile)) {
                invalidPath = false;
            } else {
                numTries++;
                std::cout << "Invalid path!" << std::endl;
                DEBUG(initialFile << std::endl);
                if (numTries >= 5) {
                    exit(1);
                }
            }
        }
    }
    std::size_t trimPos;
#if !GENERATE_FINAL_STATE
    if (finalFile.empty()) {
        if ((trimPos = initialFile.find("_initial")) != std::string::npos) {
            finalFile = initialFile;
            finalFile.erase(trimPos, 8);
            finalFile.insert(trimPos, "_final");
        }
    }

    if (finalFile.empty() || !std::filesystem::exists(finalFile)) {
        std::cout << "Path to final state:" << std::endl;
        int numTries = 0;
        bool invalidPath = true;
        while (invalidPath) {
            std::cin >> finalFile;
            if (std::filesystem::exists(finalFile)) {
                invalidPath = false;
            } else {
                numTries++;
                std::cout << "Invalid path!" << std::endl;
                DEBUG(finalFile << std::endl);
                if (numTries >= 5) {
                    exit(1);
                }
            }
        }
    }
#endif

    // Generate names for export and analysis files if they are not specified
    if (exportFile.empty()) {
        exportFile = std::filesystem::path(initialFile).replace_extension(".scen").string();
        if ((trimPos = exportFile.find("_initial")) != std::string::npos) {
            exportFile.erase(trimPos, 8);
        }
    }
    if (analysisFile.empty()) {
        auto initialFilePath = std::filesystem::path(initialFile);
        analysisFile = initialFilePath.replace_filename(initialFilePath.stem().string() + "_analysis.json").string();
        if ((trimPos = analysisFile.find("_initial")) != std::string::npos) {
            analysisFile.erase(trimPos, 8);
        }
    }

    // Dynamically Link Properties
    std::cout << "Linking Properties..." << std::endl;
    ModuleProperties::LinkProperties();
    std::cout << "Properties successfully linked: " << ModuleProperties::PropertyCount() << std::endl;

#if CONFIG_MOD_DATA_STORAGE == MM_DATA_INT64
    if (ModuleProperties::PropertyCount() > 1) {
        std::cerr << "Modules cannot be represented as integer with more than 1 property loaded." << std::endl;
    }
#endif

    // Preprocess moves
    if (movesFolder.empty()) {
        MoveManager::PreprocessMoves();
    } else {
        MoveManager::PreprocessMoves(movesFolder);
    }

    // Preprocess configurations
    LatticeSetup::Preprocess(initialFile, finalFile);

    // Set up Lattice
    std::cout << "Initializing Lattice..." << std::endl;
    // Set up edge-check override if needed
#if !LATTICE_OLD_EDGECHECK
    if (!edgeCheck.empty()) {
        std::cout << "Setting Adjacency Check Override..." << std::endl;
        if (edgeCheck == "cube" || edgeCheck == "Cube" || edgeCheck == "manhattan" || edgeCheck == "Manhattan") {
            LatticeSetup::adjCheckOverride = CUBE;
        } else if (edgeCheck == "rhombic dodecahedron" || edgeCheck == "Rhombic Dodecahedron" || edgeCheck == "rd" || edgeCheck == "RD") {
            LatticeSetup::adjCheckOverride = RHOMDOD;
        } else {
            std::cerr << "Received invalid adjacency check option: " << edgeCheck << ", using cube adjacency." << std::endl;
            LatticeSetup::adjCheckOverride = CUBE;
        }
        std::cout << "Adjacency check override set." << std::endl;
    }
#endif
    Lattice::SetFlags(ignoreColors);
    LatticeSetup::SetupFromJson(initialFile);
    std::cout << "Lattice initialized." << std::endl;

#if CONFIG_PARALLEL_MOVES
    if (ModuleIdManager::MinStaticID() > 64) {
        std::cerr << "Parallel moves currently only support up to 64 non-static modules." << std::endl;
    }
#endif
    
    // Set up moves
    std::cout << "Initializing Move Manager..." << std::endl;
    MoveManager::InitMoveManager(Lattice::Order(), Lattice::AxisSize());
    std::cout << "Move Manager initialized." << std::endl << "Loading Moves..." << std::endl;
    if(movesFolder.empty()) {
        MoveManager::RegisterAllMoves();
    } else {
        MoveManager::RegisterAllMoves(movesFolder);
    }
    std::cout << "Moves loaded." << std::endl;

    // Print some useful information
    std::cout << std::endl << "Module Representation: ";
#if CONFIG_MOD_DATA_STORAGE == MM_DATA_FULL
    std::cout << "FULL" << std::endl;
#elif CONFIG_MOD_DATA_STORAGE == MM_DATA_INT64
    std::cout << "INT64" << std::endl;
#else
    std::cout << "INVALID" << std::endl;
#endif
    std::cout << "Final State Generator: ";
#if GENERATE_FINAL_STATE
    std::cout << "ENABLED" << std::endl;
#else
    std::cout << "DISABLED" << std::endl;
#endif
    std::cout << "Edge Check Mode:       ";
#if LATTICE_OLD_EDGECHECK
#if LATTICE_RD_EDGECHECK
    std::cout << "RHOMBIC DODECAHEDRON FACE" << std::endl;
#else
    std::cout << "CUBE FACE" << std::endl;
#endif
#else
    std::cout << "GENERAL" << std::endl;
#endif
    std::cout << "Parallel Pathfinding:  ";
#if CONFIG_PARALLEL_MOVES
    std::cout << "ENABLED" << std::endl;
#else
    std::cout << "DISABLED" << std::endl;
#endif
    std::cout << "Search Method:         ";
    if (searchMethod.empty() || searchMethod == "A*" || searchMethod == "a*" || searchMethod == "BDA*" || searchMethod == "bda*") {
        std::cout << (searchMethod == "BDA*" || searchMethod == "bda*" ? "BDA*" : "A*") << std::endl;
        std::cout << "└Heuristic:            ";
        if (heuristic.empty() || heuristic == "MRSH1" || heuristic == "mrsh1" || heuristic == "MRSH-1" || heuristic == "mrsh-1") {
            std::cout << "MRSH-1" << std::endl;
            std::cout << " ├Unreachable Cache:   ";
#if CONFIG_HEURISTIC_CACHE_OPTIMIZATION
            std::cout << "ENABLED" << std::endl;
#else
            std::cout << "DISABLED" << std::endl;
#endif
            std::cout << " ├Reach Limits:        ";
#if CONFIG_HEURISTIC_CACHE_DIST_LIMITATIONS
            std::cout << "ENABLED" << std::endl;
#else
            std::cout << "DISABLED" << std::endl;
#endif
            std::cout << " └Help Limits:         ";
#if CONFIG_HEURISTIC_CACHE_HELP_LIMITATIONS
            std::cout << "ENABLED" << std::endl;
#else
            std::cout << "DISABLED" << std::endl;
#endif
        } else if (heuristic == "Symmetric Difference" || heuristic == "symmetric difference" || heuristic == "SymDiff" || heuristic == "symdiff") {
            std::cout << "Symmetric Difference" << std::endl;
        } else if (heuristic == "Manhattan" || heuristic == "manhattan") {
            std::cout << "Center of Mass Manhattan" << std::endl;
        } else if (heuristic == "Chebyshev" || heuristic == "chebyshev") {
            std::cout << "Center of Mass Chebyshev" << std::endl;
        } else if (heuristic == "Nearest Chebyshev" || heuristic == "nearest chebyshev") {
            std::cout << "Nearest Chebyshev" << std::endl;
        }
    } else if (searchMethod == "BFS" || searchMethod == "bfs") {
        std::cout << "BFS" << std::endl;
    } else if (searchMethod == "BDBFS" || searchMethod == "bdbfs") {
        std::cout << "BDBFS" << std::endl;
    }
    std::cout << std::endl;

    // Pathfinding
    Configuration start(Lattice::GetModuleInfo());
    BDConfiguration bidirectionalStart(start.GetModData(), START);
#if GENERATE_FINAL_STATE
    Configuration end = ConfigurationSpace::GenerateRandomFinal();
#else
    Configuration end = LatticeSetup::SetupFinalFromJson(finalFile);
#endif
    BDConfiguration bidirectionalEnd(end.GetModData(), END);
    std::vector<const Configuration*> path;
    try {
        std::cout << "Beginning search..." << std::endl;
        const auto timeBegin = std::chrono::high_resolution_clock::now();
        if (searchMethod.empty() || searchMethod == "A*" || searchMethod == "a*") {
            path = ConfigurationSpace::AStar(&start, &end, heuristic);
        } else if (searchMethod == "BDA*" || searchMethod == "bda*") {
            path = ConfigurationSpace::BDAStar(&bidirectionalStart, &bidirectionalEnd, heuristic);
        } else if (searchMethod == "BDBFS" || searchMethod == "bdbfs") {
            path = ConfigurationSpace::BiDirectionalBFS(&bidirectionalStart, &bidirectionalEnd);
        } else if (searchMethod == "BFS" || searchMethod == "bfs") {
            path = ConfigurationSpace::BFS(&start, &end);
        }
        const auto timeEnd = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeBegin);
        std::cout << "Search completed in " << duration.count() << " ms." << std::endl;
#if CONFIG_OUTPUT_JSON
        SearchAnalysis::ExportData(analysisFile);
#endif
    } catch(SearchExcept& searchExcept) {
        std::cerr << searchExcept.what() << std::endl;
    }

#if PRINT_PATH
    std::cout << "Path:\n";
    for (const auto config : path) {
        Lattice::UpdateFromModuleInfo(config->GetModData());
        std::cout << Lattice::ToString();
    }
#endif

    std::cout << "Exporting results..." << std::endl;
    Scenario::ScenInfo scenInfo;
    scenInfo.exportFile = exportFile;
    scenInfo.scenName = Scenario::TryGetScenName(initialFile);
    scenInfo.scenDesc = Scenario::TryGetScenDesc(initialFile);
    scenInfo.scenType = Scenario::TryGetScenType(initialFile);
    
    Scenario::ExportToScenFile(path, scenInfo);
    std::cout << "Results exported." << std::endl << "Cleaning Modules..." << std::endl;
    ModuleIdManager::CleanupModules();
    std::cout << "Modules cleaned." << std::endl << "Cleaning Moves..." << std::endl;
    Isometry::CleanupTransforms();
    std::cout << "Moves cleaned." << std::endl;
    return 0;
}