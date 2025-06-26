#include <iostream>
#include "../lattice/Lattice.h"
#include "../modules/ModuleManager.h"
#include "../coordtensor/CoordTensor.h"
#include "../lattice/Lattice.h"

class LocateAndFree {
public:
    // Access adjList from Lattice
    static std::vector<std::vector<int>>* GetAdjList() {
        return &Lattice::adjList;
    };

    // Access moduleCount from Lattice
    static int GetModuleCount() {
        return Lattice::moduleCount;
    };

    // Locate and free algorithm
    static void LocAndFree();

    // Use DFS to locate leaf nodes in the lattice
    static void DFSLeafNode(int u, std::vector<bool>& visited, std::vector<int>& parent, bool* found, std::vector<std::vector<int>>* adjList );
};