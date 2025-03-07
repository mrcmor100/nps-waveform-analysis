#include <iostream>
#include <TTree.h>
#include "ConfigManager.hpp"
#include "FileManager.hpp"
#include "BranchManager.hpp"
#include "ReferenceManager.hpp"

int main() {
    
    int run = 5200;
    int block = 10;
    ConfigManager config("config/config.json");

    FileManager fileManager(config);
    TChain* tree = fileManager.LoadTChain(run);
    TFile* outputFile = fileManager.CreateOutputFile(run, 0);

    BranchManager branchManager(tree, config);
    ReferenceManager refManager(config);

    branchManager.PrintLoadedBranches();

    auto* interpolator = refManager.GetInterpolator(block);
    if (interpolator) {
        std::cout << "Interpolator loaded for block " << block << std::endl;
    } else {
        std::cout << "Interpolator not available for block " << block << std::endl;
    }

    delete tree;
    return 0;
}
