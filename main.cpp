#include <iostream>
#include <string>
#include <stdexcept>
#include "ConfigManager.hpp"
#include "FileManager.hpp"
#include "BranchManager.hpp"
#include "GlobalManager.hpp"
#include "ReferenceManager.hpp"

// Display usage instructions.
void printUsage(const char* exeName) {
    std::cout << "Usage: " << exeName << " <run> [config_file]\n";
    std::cout << "  <run>         : Required run number (integer).\n";
    std::cout << "  [config_file] : Optional path to configuration file (default: config/config.json).\n";
}

int main(int argc, char* argv[]) {
    // Check if the required run parameter is provided.
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    int run;
    try {
        run = std::stoi(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "Error: Invalid run number provided. " << e.what() << "\n";
        printUsage(argv[0]);
        return 1;
    }

    // Use the provided config file or default to "config/config.json"
    std::string configFile = (argc >= 3) ? argv[2] : "config/config.json";

    try {
        // Load configuration.
        ConfigManager config(configFile, run);

        // Retrieve configuration structs.
        const GlobalConfig& globalCfg = config.GetGlobalConfig();
        const BranchConfig& branchCfg = config.GetBranchConfig();
        const FileIOConfig& fileCfg   = config.GetFileIOConfig();
        const ReferenceConfig& refCfg = config.GetReferenceConfig();

        // Create and use managers.
        FileManager fileManager(fileCfg);
        TChain* tree = fileManager.LoadTChain(run);
        TFile* outputFile = fileManager.CreateOutputFile(run, 0);

        BranchManager branchManager(tree, branchCfg);
        GlobalManager globalManager(globalCfg);
        ReferenceManager refManager(run, globalCfg, refCfg);

        // Example output.
        std::cout << "Global nblocks: " << globalManager.GetNblocks() << "\n";
        branchManager.PrintLoadedBranches();

        // Load reference waveforms.
        if (!refManager.LoadReferenceWaveforms()) {
            std::cerr << "Failed to load reference waveforms.\n";
        }

        // Example: Retrieve an interpolator for a specific block.
        int block = 10;
        auto* interpolator = refManager.GetInterpolator(block);
        if (interpolator) {
            std::cout << "Interpolator loaded for block " << block << "\n";
        } else {
            std::cout << "Interpolator not available for block " << block << "\n";
        }

        // Cleanup.
        delete tree;
    } catch (const std::exception& e) {
        std::cerr << "Error encountered: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
