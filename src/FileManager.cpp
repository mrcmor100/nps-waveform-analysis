#include "FileManager.hpp"

FileManager::FileManager(ConfigManager& configManager)
    : config(configManager) {
    basePath = config.GetString("io_settings.base_path");
    inputSubdir = config.GetString("io_settings.input_subdir");
    outputSubdir = config.GetString("io_settings.output_subdir");
    inputPattern = config.GetString("io_settings.input_rootfile");
    outputPattern = config.GetString("io_settings.output_rootfile");
}

std::vector<int> FileManager::GetSegments() {
    return config.GetSegments();
}

std::string FileManager::GetInputFilePath(int run, int segment) {
    if (inputPattern.empty()) {
        std::cerr << "Error: Input file pattern is empty in FileManager." << std::endl;
        return "";
    }

    char buffer[256];
    int ret = snprintf(buffer, sizeof(buffer), inputPattern.c_str(), run, segment);

    if (ret < 0 || ret >= static_cast<int>(sizeof(buffer))) {
        std::cerr << "Error: snprintf failed or truncated output for pattern: " << inputPattern << std::endl;
        return "";
    }

    std::string filePath = basePath + inputSubdir + buffer;
    return filePath;
}

std::string FileManager::GetOutputFilePath(int run, int segment) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), outputPattern.c_str(), run, segment);
    return basePath + outputSubdir + buffer;
}

TChain* FileManager::LoadTChain(int run) {
    TChain* chain = new TChain(config.GetString("io_settings.input_tree").c_str());
    for (int segment : GetSegments()) {
        std::string filename = GetInputFilePath(run, segment);
        std::cout << filename << std::endl;
        TFile* file = TFile::Open(filename.c_str());

        if (file && !file->IsZombie()) {
            chain->Add(filename.c_str());
            std::cout << "Loaded segment " << segment << " with " << chain->GetEntries() << " entries." << std::endl;
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
