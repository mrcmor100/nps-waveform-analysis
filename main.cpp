#include <iostream>
#include <TTree.h>
#include "ConfigManager.hpp"
#include "BranchManager.hpp"
#include "ReferenceManager.hpp"

int main() {
    ConfigManager config("config/config.json");

    // Input File(s) manager
    //TTree* tree = new TTree("T", "Example Tree");

    BranchManager branchManager(tree, config);
    ReferenceManager refManager(config);

    branchManager.PrintLoadedBranches();

    int run = 5200;
    int block = 10;
    std::string waveformFile = refManager.GetReferenceWaveform(run, block);
    std::cout << "Waveform file for run " << run << ", block " << block << ": " << waveformFile << std::endl;

    delete tree;
    return 0;
}
