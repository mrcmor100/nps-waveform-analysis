#include "FileManager.hpp"

FileManager::FileManager(const FileIOConfig& fileCfg)
    : fileConfig(fileCfg)
{
}

std::string FileManager::ResolvePath(const std::string& pattern, int value1, int value2) {
    // Build the full path using the base path and the formatted pattern.
    std::string fullPath = fileConfig.basePath + fileConfig.inputSubdir + pattern;
    char buffer[256];
    snprintf(buffer, sizeof(buffer), fullPath.c_str(), value1, value2);
    return std::string(buffer);
}

std::string FileManager::GetInputFilePath(int run, int segment) {
    if (fileConfig.inputPattern.empty()) {
        std::cerr << "Error: Input file pattern is empty in FileManager." << std::endl;
        return "";
    }
    return ResolvePath(fileConfig.inputPattern, run, segment);
}

std::string FileManager::GetOutputFilePath(int run, int segment) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), fileConfig.outputPattern.c_str(), run, segment);
    return fileConfig.basePath + fileConfig.outputSubdir + std::string(buffer);
}

TChain* FileManager::LoadTChain(int run) {
    TChain* chain = new TChain(fileConfig.inputTree.c_str());
    // Here we assume a default list of segments.
    std::vector<int> segments = {0, 1, 2, 5};
    for (int seg : segments) {
        std::string filename = GetInputFilePath(run, seg);
        std::cout << "Loading file: " << filename << std::endl;
        TFile* file = TFile::Open(filename.c_str());
        if (file && !file->IsZombie()) {
            chain->Add(filename.c_str());
            std::cout << "Loaded segment " << seg << " with " << chain->GetEntries() << " entries." << std::endl;
            file->Close();
        } else {
            std::cerr << "Warning: File does not exist or is corrupted: " << filename << std::endl;
        }
    }
    return chain;
}

TFile* FileManager::CreateOutputFile(int run, int segment) {
    std::string rootfilePath = GetOutputFilePath(run, segment);
    std::cout << "Creating output file: " << rootfilePath << std::endl;
    return new TFile(rootfilePath.c_str(), "recreate");
}
