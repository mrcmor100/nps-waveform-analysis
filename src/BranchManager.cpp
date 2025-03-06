#include "BranchManager.hpp"

BranchManager::BranchManager(TTree* tree, ConfigManager& configManager) : tree(tree) {
    auto branches = configManager.GetBranchVariableMap();
    for (const auto& [branchName, varName] : branches) {
        tree->SetBranchStatus(branchName.c_str(), 1);
        double* var = new double;
        allocatedMemory[branchName] = var;
        tree->SetBranchAddress(branchName.c_str(), var);
        namedVariables[varName] = *var;
    }
}

void BranchManager::PrintLoadedBranches() {
    std::cout << "Loaded branches:\n";
    for (const auto& [name, var] : namedVariables) {
        std::cout << " - " << name << std::endl;
    }
}
