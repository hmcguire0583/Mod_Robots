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
#include "LocateAndFree/LocateAndFree.h"


int main(int argc, char* argv[]) {
    std::string initialFile;
    std::string finalFile;
    std::string exportFile;
    bool exportRequested = false;  // Track if -e flag was used
    //
    //

    // Define the long options
    static option long_options[] = {
        {"initial-file", required_argument, nullptr, 'I'},
        {"final-file", required_argument, nullptr, 'F'},
        {"export-file", required_argument, nullptr, 'e'},
    };

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "I:F:e:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'I':
                initialFile = optarg;
                break;
            case 'F':
                finalFile = optarg;
                break;
            case 'e':
                exportFile = optarg;
                exportRequested = true;  // Mark that export was requested
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

    // Only generate export file name if -e flag was used but no filename provided
    if (exportRequested && exportFile.empty()) {
        exportFile = std::filesystem::path(initialFile).replace_extension(".scen").string();
        if ((trimPos = exportFile.find("_initial")) != std::string::npos) {
            exportFile.erase(trimPos, 8);
        }
    }
    
    // Preprocess configurations
    LatticeSetup::Preprocess(initialFile, finalFile);

    // Set up Lattice
    std::cout << "Initializing Lattice..." << std::endl;

    LatticeSetup::adjCheckOverride = CUBE;

    LatticeSetup::SetupFromJson(initialFile);
    std::cout << "Lattice initialized." << std::endl;
    // Print initial lattice state
    std::cout << "\nInitial Lattice State:" << std::endl;
    std::cout << Lattice::ToString() << std::endl;

    // Store initial configuration for export (only if export requested)
    Configuration* start = nullptr;
    if (exportRequested) {
        start = new Configuration(Lattice::GetModuleInfo());
    }

    LocateAndFree::LocAndFree();

    // Export to scen file only if -e flag was used
    if (exportRequested) {
        // Store final configuration
        Configuration end(Lattice::GetModuleInfo());

        // Create path with initial and final configurations
        std::vector<const Configuration*> path;
        path.push_back(start);
        path.push_back(&end);

        std::cout << "Exporting results..." << std::endl;
        Scenario::ScenInfo scenInfo;
        scenInfo.exportFile = exportFile;
        scenInfo.scenName = Scenario::TryGetScenName(initialFile);
        scenInfo.scenDesc = Scenario::TryGetScenDesc(initialFile);
        scenInfo.scenType = Scenario::TryGetScenType(initialFile);
        
        Scenario::ExportToScenFile(path, scenInfo);
        std::cout << "Results exported to: " << exportFile << std::endl;

        delete start;
    }

    return 0;
}