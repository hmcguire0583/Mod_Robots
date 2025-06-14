#include <iostream>
#include <nlohmann/json.hpp>  
#include "Lattice.h"
#include "ModuleManager.h"

int main() {
    Lattice::InitLattice(3, 10, 1);
    Lattice::SetAdjIndicesFromOffsets(LatticeUtils::cubeAdjOffsets);

    // Create JSON property definitions
    nlohmann::json props1 = { {"color", "red"}, {"type", "core"} };
    nlohmann::json props2 = { {"color", "blue"}, {"type", "limb"} };

    // Register modules
    ModuleIdManager::RegisterModule({5, 5, 5}, false, props1, false);
    ModuleIdManager::RegisterModule({6, 5, 5}, false, props2, false);

    // Add to lattice
    for (auto& mod : ModuleIdManager::Modules()) {
        Lattice::AddModule(mod);
    }

    std::cout << "Lattice connectivity: " << (Lattice::CheckConnected(0) ? "Connected" : "Disconnected") << std::endl;
    return 0;
}
