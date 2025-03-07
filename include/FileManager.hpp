#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <TFile.h>
#include <TChain.h>
#include "ConfigManager.hpp"

namespace fs = std::filesystem;

class FileManager {
public:
    explicit FileManager(ConfigManager& configManager);

    std::vector<int> GetSegments();
    std::string GetInputFilePath(int run, int segment);
    std::string GetOutputFilePath(int run, int segment);
    TChain* LoadTChain(int run);
    TFile* CreateOutputFile(int run, int segment);

private:
    ConfigManager& config;
    std::string basePath;
    std::string inputSubdir;
    std::string outputSubdir;
    std::string inputPattern;
    std::string outputPattern;
};
