#include <iostream>
#include "ConfigManager.hpp"
#include "FileManager.hpp"
#include "BranchManager.hpp"
#include "GlobalManager.hpp"
#include "ReferenceManager.hpp"  // if needed

int main() {
    int run = 5200;
    // Instantiate the ConfigManager with a configuration file and run number.
    ConfigManager config("config/config.json", run);

    // Retrieve the configuration structs.
    const GlobalConfig& globalCfg = config.GetGlobalConfig();
    const BranchConfig& branchCfg = config.GetBranchConfig();
    const FileIOConfig& fileCfg   = config.GetFileIOConfig();

    // Create managers with their respective configuration structs.
    FileManager fileManager(fileCfg);
    TChain* tree = fileManager.LoadTChain(run);
    TFile* outputFile = fileManager.CreateOutputFile(run, 0);

    BranchManager branchManager(tree, branchCfg);
    GlobalManager globalManager(globalCfg);

    // Optionally, you can now query global parameters via GlobalManager.
    std::cout << "Global nblocks: " << globalManager.GetNblocks() << std::endl;
    
    branchManager.PrintLoadedBranches();
    
    // Cleanup.
    delete tree;
    return 0;
}
