#include <iostream>
#include <vector>
#include <stack>
#include "../lattice/Lattice.h"
#include "../modules/ModuleManager.h"
#include "../coordtensor/CoordTensor.h"

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

    // Main Locate and Free algorithm
    static void LocAndFree();

    // Find the starting module S (Greatest Z, then smallest X & Y)
    static int FindStartingModule();

    // Use DFS to locate leaf nodes in the lattice
    static void DFSLeafNode(int u, std::vector<bool>& visited, std::vector<int>& parent, bool* found, std::vector<std::vector<int>>* adjList);

    // Find path from start to leaf using DFS
    static std::vector<int> FindPathToLeaf(int start, int leaf);

    // Move a module one step towards the smallest coordinates
    static void MoveModuleTowardsSmallest(int moduleId);

    // Check if a module can be moved in a given direction
    static bool CanMoveModule(int moduleId, const std::valarray<int>& direction);

    // Get the direction towards smallest coordinates
    static std::valarray<int> GetDirectionToSmallest(const std::valarray<int>& currentPos);
};