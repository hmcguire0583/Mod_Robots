#include "LocateAndFree.h"
#include <algorithm>
#include <limits>

void LocateAndFree::LocAndFree() {
    std::cout << "Starting Locate and Free Algorithm..." << std::endl;
    
    // Step 1: Find the starting module S (Greatest Z, then smallest X & Y)
    int startModule = FindStartingModule();
    std::cout << "Starting module S: " << startModule << std::endl;
    
    // Step 2: Use DFS to find a leaf node
    auto moduleCount = GetModuleCount();
    std::vector<bool> visited(moduleCount, false);
    std::vector<int> parent(moduleCount, -1);
    bool foundLeaf = false;
    std::vector<std::vector<int>>* adjList = GetAdjList();
    
    int leafNode = -1;
    
    // Find leaf node using DFS
    std::stack<int> dfsStack;
    dfsStack.push(startModule);
    visited[startModule] = true;
    
    while (!dfsStack.empty() && !foundLeaf) {
        int current = dfsStack.top();
        dfsStack.pop();
        
        bool hasUnvisitedChildren = false;
        
        for (int neighbor : (*adjList)[current]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                parent[neighbor] = current;
                dfsStack.push(neighbor);
                hasUnvisitedChildren = true;
            }
        }
        
        // If no unvisited children, this is a leaf
        if (!hasUnvisitedChildren) {
            leafNode = current;
            foundLeaf = true;
            std::cout << "Leaf node found: " << leafNode << std::endl;
        }
    }
    
    if (!foundLeaf) {
        std::cout << "No leaf node found!" << std::endl;
        return;
    }
    
    // Step 3: Find path from start to leaf
    std::vector<int> pathToLeaf = FindPathToLeaf(startModule, leafNode);
    
    std::cout << "Path from start to leaf: ";
    for (int i = 0; i < pathToLeaf.size(); i++) {
        std::cout << pathToLeaf[i];
        if (i < pathToLeaf.size() - 1) std::cout << " -> ";
    }
    std::cout << std::endl;
    
    // Step 4: Move the leaf module towards smallest coordinates
    std::cout << "Moving leaf module towards smallest coordinates..." << std::endl;
    MoveModuleTowardsSmallest(leafNode);
    
    std::cout << "Locate and Free Algorithm completed." << std::endl;
}

int LocateAndFree::FindStartingModule() {
    int startModule = -1;
    int maxZ = std::numeric_limits<int>::min();
    int minX = std::numeric_limits<int>::max();
    int minY = std::numeric_limits<int>::max();
    
    // Iterate through all non-static modules
    for (int id = 0; id < ModuleIdManager::MinStaticID(); id++) {
        const auto& module = ModuleIdManager::GetModule(id);
        const auto& coords = module.coords;
        
        int z = (coords.size() > 2) ? coords[2] : 0;
        int x = coords[0];
        int y = coords[1];
        
        // Find module with greatest Z, then smallest X & Y
        bool isNewStart = false;
        
        if (z > maxZ) {
            isNewStart = true;
        } else if (z == maxZ) {
            if (x < minX) {
                isNewStart = true;
            } else if (x == minX && y < minY) {
                isNewStart = true;
            }
        }
        
        if (isNewStart) {
            startModule = id;
            maxZ = z;
            minX = x;
            minY = y;
        }
    }
    
    return startModule;
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

std::vector<int> LocateAndFree::FindPathToLeaf(int start, int leaf) {
    std::vector<int> path;
    auto moduleCount = GetModuleCount();
    std::vector<bool> visited(moduleCount, false);
    std::vector<int> parent(moduleCount, -1);
    std::vector<std::vector<int>>* adjList = GetAdjList();
    
    // BFS to find path
    std::queue<int> queue;
    queue.push(start);
    visited[start] = true;
    
    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();
        
        if (current == leaf) {
            // Reconstruct path
            int node = leaf;
            while (node != -1) {
                path.push_back(node);
                node = parent[node];
            }
            std::reverse(path.begin(), path.end());
            break;
        }
        
        for (int neighbor : (*adjList)[current]) {
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                parent[neighbor] = current;
                queue.push(neighbor);
            }
        }
    }
    
    return path;
}

void LocateAndFree::MoveModuleTowardsSmallest(int moduleId) {
    auto& module = ModuleIdManager::GetModule(moduleId);
    std::valarray<int> currentPos = module.coords;
    
    std::cout << "Current position of module " << moduleId << ": ";
    for (int i = 0; i < currentPos.size(); i++) {
        std::cout << currentPos[i];
        if (i < currentPos.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Get direction towards smallest coordinates
    std::valarray<int> direction = GetDirectionToSmallest(currentPos);
    
    std::cout << "Direction towards smallest: ";
    for (int i = 0; i < direction.size(); i++) {
        std::cout << direction[i];
        if (i < direction.size() - 1) std::cout << ", ";
    }
    std::cout << std::endl;
    
    // Check if we can move in this direction
    if (CanMoveModule(moduleId, direction)) {
        // Update module position
        std::valarray<int> newPos = currentPos + direction;
        
        // Update lattice
        Lattice::coordTensor[currentPos] = FREE_SPACE;
        module.coords = newPos;
        Lattice::coordTensor[newPos] = moduleId;
        
        std::cout << "Module " << moduleId << " moved to: ";
        for (int i = 0; i < newPos.size(); i++) {
            std::cout << newPos[i];
            if (i < newPos.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        // Update adjacency information
        Lattice::ClearAdjacencies(moduleId);
        Lattice::EdgeCheck(module);
    } else {
        std::cout << "Cannot move module " << moduleId << " in direction towards smallest coordinates." << std::endl;
    }
}

bool LocateAndFree::CanMoveModule(int moduleId, const std::valarray<int>& direction) {
    const auto& module = ModuleIdManager::GetModule(moduleId);
    std::valarray<int> newPos = module.coords + direction;
    
    // Check bounds
    for (int i = 0; i < newPos.size(); i++) {
        if (newPos[i] < 0 || newPos[i] >= Lattice::AxisSize()) {
            return false;
        }
    }
    
    // Check if target position is free
    int targetContent = Lattice::coordTensor[newPos];
    if (targetContent != FREE_SPACE) {
        return false;
    }
    
    return true;
}

std::valarray<int> LocateAndFree::GetDirectionToSmallest(const std::valarray<int>& currentPos) {
    std::valarray<int> direction(0, currentPos.size());
    
    // Move towards (0, 0, 0) - the smallest coordinates
    for (int i = 0; i < currentPos.size(); i++) {
        if (currentPos[i] > 0) {
            direction[i] = -1;  // Move towards smaller coordinate
            break;  // Only move in one direction at a time
        }
    }
    
    return direction;
}