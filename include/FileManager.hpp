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
    // The API now requires file patterns to use the <run> and <segment> keywords.
    FileManager(const FileIOConfig& fileCfg);
    TChain* LoadTChain(int run);
    TFile* CreateOutputFile(int run, int segment);
private:
    FileIOConfig fileConfig;
    // Helper functions for path resolution.
    std::string ResolvePath(const std::string& pattern, int run, int segment);
    std::string GetInputFilePath(int run, int segment);
    std::string GetOutputFilePath(int run, int segment);
    std::vector<int> DetectSegments(int run) const;
    // Helper: escapes regex metacharacters except those within placeholders (<...>)
    std::string EscapeRegexExceptPlaceholders(const std::string& pattern) const;
};

#endif // FILE_MANAGER_HPP
