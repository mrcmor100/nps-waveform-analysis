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

    std::vector<int> segments = fileConfig.inputSegments;

    if (fileConfig.inputSegments.size() == 1 && fileConfig.inputSegments[0] == -1) {
        segments = DetectSegments(run);
    } else {
        segments = fileConfig.inputSegments;
    }

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

namespace fs = std::filesystem;

std::vector<int> FileManager::DetectSegments(int run) const {
    std::vector<int> segments;
    fs::path replayPath = fs::path(fileConfig.basePath) / fileConfig.inputSubdir;

    // Create a regex from the input pattern: replace %d with regex groups
    // Example: nps_hms_coin_%d_%d_1_-1.root -> nps_hms_coin_<run>_(\d+)_1_-1.root
    char prefixBuf[256];
    snprintf(prefixBuf, sizeof(prefixBuf), fileConfig.inputPattern.c_str(), run, 0); // dummy segment=0
    std::string dummyFilename(prefixBuf);

    // Build a regex from dummyFilename by replacing segment "0" with a capture group
    std::string pattern = dummyFilename;
    // TODO: Fix hard coded _0_1_-1.root.  If this ending changes, the code will fail.
    std::size_t pos = pattern.find("_0_1_-1.root");
    if (pos == std::string::npos) {
        std::cerr << "Could not parse input pattern for regex matching.\n";
        return segments;
    }

    std::string regexStr = pattern.substr(0, pos) + "_(\\d+)_1_-1\\.root";
    std::regex fileRegex(regexStr);

    for (const auto& entry : fs::directory_iterator(replayPath)) {
        if (!entry.is_regular_file()) continue;

        const std::string filename = entry.path().filename().string();
        std::smatch match;
        if (std::regex_match(filename, match, fileRegex)) {
            if (match.size() == 2) {
                int seg = std::stoi(match[1].str());
                segments.push_back(seg);
            }
        }
    }

    std::sort(segments.begin(), segments.end());
    return segments;
}
