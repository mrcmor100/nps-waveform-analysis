#ifndef BRANCH_MANAGER_HPP
#define BRANCH_MANAGER_HPP

#include "ManagerConfigs.hpp"
#include <TTree.h>
#include <unordered_map>
#include <string>
#include <iostream>

class BranchManager {
public:
    // Now accepts a BranchConfig struct.
    BranchManager(TTree* tree, const BranchConfig& branchCfg);
    ~BranchManager();
    void PrintLoadedBranches() const;
    void ApplyConfig(int _run);
private:
    int run;
    TTree* tree;
    // Map to hold allocated memory for each branch.
    std::unordered_map<std::string, double*> allocatedMemory;
    // Map to store the variable names associated with each branch.
    std::unordered_map<std::string, double> namedVariables;
};

#endif // BRANCH_MANAGER_HPP
