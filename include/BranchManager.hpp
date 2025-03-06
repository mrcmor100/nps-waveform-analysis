#pragma once
#include <iostream>
#include <unordered_map>
#include <vector>
#include <TTree.h>
#include "ConfigManager.hpp"

class BranchManager {
public:
    BranchManager(TTree* tree, ConfigManager& configManager);
    void DefineBranches();
    void PrintLoadedBranches();
    
private:
    TTree* tree;
    std::unordered_map<std::string, void*> allocatedMemory;
    std::unordered_map<std::string, int> intVariables;
    std::unordered_map<std::string, double> namedVariables;
};
