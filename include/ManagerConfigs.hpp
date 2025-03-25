#ifndef MANAGER_CONFIGS_HPP
#define MANAGER_CONFIGS_HPP

#include <string>
#include <vector>
#include <unordered_map>

enum class RunType {
    Unknown,
    Prod,
    Elastic
};

struct ApplicationConfig {
    int nProcessors;
    std::string version;
};

struct GlobalRunOverride {
    int runLow;
    int runHigh;
    // Example overrides; add more as needed.
    int ntimeOverride = 0;
    int timerefaccOverride = 0;
};

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
    float timerefacc;
    double peakTolerance;

    std::vector<GlobalRunOverride> runOverrides;
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
    std::string outputTree;
    std::vector<int> inputSegments;
};

// A small structure for file pattern ranges.
struct FilePattern {
    int rangeStart;
    int rangeEnd;
    int elasticStart;
    int elasticEnd;
    std::string path;

    bool MatchesProd(int run) const {
        return run >= rangeStart && run <= rangeEnd;
    }

    bool MatchesElastic(int run) const {
        return run >= elasticStart && run <= elasticEnd;
    }

    bool MatchesAny(int run) const {
        return MatchesProd(run) || MatchesElastic(run);
    }
};

struct ChannelType {
    int chStart;
    int chEnd;
    std::string channelName;
};

// Reference configuration structure. In addition to run-dependent
// global parameters (which are also in GlobalConfig), we add settings that are
// specific to the reference manager.
struct ReferenceConfig {
    // Waveform file patterns and default.
    float timemean;
    float timemean2;
    std::vector<FilePattern> waveformPatterns;
    std::vector<ChannelType> ChannelTypes;
    std::string waveformDefault;
};

#endif // MANAGER_CONFIGS_HPP
