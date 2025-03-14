#ifndef FILE_MANAGER_HPP
#define FILE_MANAGER_HPP

#include "ManagerConfigs.hpp"
#include <TChain.h>
#include <TFile.h>
#include <string>
#include <vector>
#include <cstdio>
#include <iostream>

class FileManager {
public:
    // Constructor now accepts a FileIOConfig struct.
    FileManager(const FileIOConfig& fileCfg);
    TChain* LoadTChain(int run);
    TFile* CreateOutputFile(int run, int segment);
private:
    FileIOConfig fileConfig;
    // Helper functions for path resolution.
    std::string ResolvePath(const std::string& pattern, int value1, int value2);
    std::string GetInputFilePath(int run, int segment);
    std::string GetOutputFilePath(int run, int segment);
};

#endif // FILE_MANAGER_HPP
