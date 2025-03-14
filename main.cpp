#include <iostream>
#include "ConfigManager.hpp"
#include "FileManager.hpp"
#include "BranchManager.hpp"
#include "ReferenceManager.hpp"

int main() {
    int run = 5200;
    // Instantiate ConfigManager with the configuration file and run number.
    ConfigManager config("config/config.json", run);

    // Retrieve the configuration structs.
    const GlobalConfig& globalCfg = config.GetGlobalConfig();
    const BranchConfig& branchCfg = config.GetBranchConfig();
    const FileIOConfig& fileCfg   = config.GetFileIOConfig();
    const ReferenceConfig& refCfg = config.GetReferenceConfig();

    // Create FileManager and BranchManager using their respective configuration structs.
    FileManager fileManager(fileCfg);
    TChain* tree = fileManager.LoadTChain(run);
    TFile* outputFile = fileManager.CreateOutputFile(run, 0);

    BranchManager branchManager(tree, branchCfg);
    // ReferenceManager now receives both global and reference configs.
    ReferenceManager refManager(run, globalCfg, refCfg);
    refManager.LoadReferenceWaveforms();

    branchManager.PrintLoadedBranches();
    
    // Example of retrieving an interpolator for a specific block.
    int block = 10;
    auto* interpolator = refManager.GetInterpolator(block);
    if (interpolator) {
        std::cout << "Interpolator loaded for block " << block << std::endl;
    } else {
        std::cout << "Interpolator not available for block " << block << std::endl;
    }
    
    delete tree;
    return 0;
}
