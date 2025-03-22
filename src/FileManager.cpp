#include "FileManager.hpp"

namespace fs = std::filesystem;

FileManager::FileManager(const FileIOConfig& fileCfg)
    : fileConfig(fileCfg)
{
}

// Escape regex special characters outside of placeholders.
// This ensures that user-provided patterns (like dots, dashes, etc.) are treated literally.
std::string FileManager::EscapeRegexExceptPlaceholders(const std::string& pattern) const {
    std::string escaped;
    bool inPlaceholder = false;
    for (char c : pattern) {
        if (c == '<') {
            inPlaceholder = true;
        }
        // If not inside a placeholder and c is a regex metacharacter, escape it.
        if (!inPlaceholder && std::string(".^$|()[]{}*+?\\").find(c) != std::string::npos) {
            escaped.push_back('\\');
        }
        escaped.push_back(c);
        if (c == '>') {
            inPlaceholder = false;
        }
    }
    return escaped;
}

// Replace <run> and <segment> in the pattern using simple string substitution.
std::string FileManager::ResolvePath(const std::string& pattern, int run, int segment) {
    std::string result = pattern;
    size_t pos;
    while ((pos = result.find("<run>")) != std::string::npos) {
        result.replace(pos, 5, std::to_string(run));
    }
    while ((pos = result.find("<segment>")) != std::string::npos) {
        result.replace(pos, 9, std::to_string(segment));
    }
    return fileConfig.basePath + fileConfig.inputSubdir + result;
}

std::string FileManager::GetInputFilePath(int run, int segment) {
    if (fileConfig.inputPattern.empty()) {
        std::cerr << "Error: Input file pattern is empty in FileManager." << std::endl;
        return "";
    }
    return ResolvePath(fileConfig.inputPattern, run, segment);
}

std::string FileManager::GetOutputFilePath(int run, int segment) {
    std::string result = fileConfig.outputPattern;
    size_t pos;
    while ((pos = result.find("<run>")) != std::string::npos) {
        result.replace(pos, 5, std::to_string(run));
    }
    while ((pos = result.find("<segment>")) != std::string::npos) {
        result.replace(pos, 9, std::to_string(segment));
    }
    return fileConfig.basePath + fileConfig.outputSubdir + result;
}

TChain* FileManager::LoadTChain(int run) {
    TChain* chain = new TChain(fileConfig.inputTree.c_str());
    std::vector<int> segments = fileConfig.inputSegments;

    // If the configuration indicates "all" segments by a sentinel (-1), detect segments dynamically.
    if (segments.size() == 1 && segments[0] == -1) {
        segments = DetectSegments(run);
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

// Detect segments using std::filesystem and regex built from the input pattern.
// The new pattern uses <run> and <segment>, so we can create a regex that matches:
// e.g. "nps_hms_coin_<run>_<segment>_1_-1.root" becomes
// "nps_hms_coin_12345_(\d{1,2})_1_-1\.root" for run=12345.
std::vector<int> FileManager::DetectSegments(int run) const {
    std::vector<int> segments;
    fs::path replayPath = fs::path(fileConfig.basePath) / fileConfig.inputSubdir;

    // Escape regex special characters in the user-supplied pattern (except placeholders).
    std::string escapedPattern = EscapeRegexExceptPlaceholders(fileConfig.inputPattern);

    // Replace <run> with the actual run number.
    escapedPattern = std::regex_replace(escapedPattern, std::regex("<run>"), std::to_string(run));
    // Replace <segment> with a capture group that accepts one or two digits.
    escapedPattern = std::regex_replace(escapedPattern, std::regex("<segment>"), R"((\d{1,2}))");

    std::regex fileRegex(escapedPattern);

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
