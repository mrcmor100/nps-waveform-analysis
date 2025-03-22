#include "BranchManager.hpp"

BranchManager::BranchManager(TTree* tree, const BranchConfig& branchCfg)
    : tree(tree), run(-1)
{
    // For every branch defined in the config, enable it and set its address.
    for (const auto& [branchName, varName] : branchCfg.branchVarMap) {
        tree->SetBranchStatus(branchName.c_str(), 1);
        double* var = new double;
        allocatedMemory[branchName] = var;
        tree->SetBranchAddress(branchName.c_str(), var);
        // Store the initial value (could later be updated during tree processing)
        namedVariables[varName] = *var;
    }
}

BranchManager::~BranchManager() {
    // Free allocated memory.
    for (auto& [branchName, ptr] : allocatedMemory) {
        delete ptr;
    }
}

void BranchManager::PrintLoadedBranches() const {
    std::cout << "Loaded branches:\n";
    for (const auto& [name, value] : namedVariables) {
        std::cout << " - " << name << std::endl;
    }
}

void BranchManager::ApplyConfig(int _run) {
    run = _run;
}