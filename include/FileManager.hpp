#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include "ManagerConfigs.hpp"
#include <TChain.h>
#include <TFile.h>
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <regex>

class FileManager {
public:
    // Constructor accepts the file I/O configuration.
    explicit FileManager(const FileIOConfig& fileCfg);

    ~FileManager();

    // Apply the configuration for the specified run.
    // This builds the input TChain and creates the output TFile.
    // Returns true if both operations succeed.
    bool ApplyConfig(int run);

    // Accessor for the input chain.
    TChain* GetInputChain() const;

    // Accessor for the output file.
    TFile* GetOutputFile() const;

private:
    // Owned ROOT resources.
    TChain* inputChain;
    TFile* outputFile;

    // The run number used (set during ApplyConfig).
    int run;
    std::vector<int> segments;
    // File I/O configuration.
    FileIOConfig fileConfig;

    // Helper functions:
    TChain* LoadTChain(int run);
    TFile* CreateOutputFile(int run, int segment);
    std::string ResolvePath(const std::string& pattern, int run, int segment);
    std::string GetInputFilePath(int run, int segment);
    std::string GetOutputFilePath(int run, int segment);
    std::vector<int> DetectSegments(int run) const;
    std::string EscapeRegexExceptPlaceholders(const std::string& pattern) const;
};

#endif // FILE_MANAGER_HPP
