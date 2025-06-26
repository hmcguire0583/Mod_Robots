#include "LocateAndFree.h"

void LocateAndFree::LocAndFree() {
    int start = 0;
    auto moduleCount = LocateAndFree::GetModuleCount();
    std::vector<bool> visited(moduleCount, false);
    std::vector<int> parent(moduleCount, -1);
    bool foundLeaf = false;
    std::vector<std::vector<int>>* adjList = LocateAndFree::GetAdjList();

    DFSLeafNode(start, visited, parent, &foundLeaf, adjList);
 }

void LocateAndFree::DFSLeafNode(int u, std::vector<bool>& visited, std::vector<int>& parent, bool* found, std::vector<std::vector<int>>* adjList) {
    if (*found) return;

    std::vector<std::vector<int>> tempAdjList = *adjList;

    visited[u] = true;

    bool hasUnvisitedChildren = false;

    for (const int v : tempAdjList[u]) {
        if (!visited[v]) {
            parent[v] = u;
            hasUnvisitedChildren = true;
            DFSLeafNode(v, visited, parent, found, adjList);
            if (*found) return;
        }
    }

    if (!hasUnvisitedChildren) {
        std::cout << "Leaf node found: " << u << std::endl;
        *found = true;
    }
}