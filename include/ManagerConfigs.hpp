#ifndef MANAGER_CONFIGS_HPP
#define MANAGER_CONFIGS_HPP

#include <string>
#include <vector>
#include <unordered_map>

// Global configuration structure (default values are in the "global" section).
struct GlobalConfig {
    int nchannel;
    int nblocks;
    int ncol;
    int nlin;
    int maxpulses;
    int maxwfpulses;
    int ntemp;
    int ntime;
    int nfADC;
    double dt;
    int nslots;
    double ADCtomV;
    double integtopC;
    double timerefacc;
};

// Branch configuration structure.
struct BranchConfig {
    std::unordered_map<std::string, std::string> branchVarMap;
    std::vector<std::string> branchNames;
};

// File I/O configuration structure.
struct FileIOConfig {
    std::string basePath;
    std::string inputSubdir;
    std::string outputSubdir;
    std::string inputPattern;
    std::string outputPattern;
    std::string inputTree;
};

// A small structure for file pattern ranges.
struct FilePattern {
    int rangeStart;
    int rangeEnd;
    std::string path;
};

// Reference configuration structure. In addition to run-dependent
// global parameters (which are also in GlobalConfig), we add settings that are
// specific to the reference manager.
struct ReferenceConfig {
    // Waveform file patterns and default.
    std::vector<FilePattern> waveformPatterns;
    std::string waveformDefault;
    // TDC file patterns and default.
    std::vector<FilePattern> tdcPatterns;
    std::string tdcDefault;
};

#endif // MANAGER_CONFIGS_HPP
