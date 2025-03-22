#include "FileManager.hpp"

namespace fs = std::filesystem;

FileManager::FileManager(const FileIOConfig& fileCfg)
    : fileConfig(fileCfg), inputChain(nullptr), outputFile(nullptr), run(-1)
{
}

FileManager::~FileManager() {
    // Clean up the output file.
    if (outputFile) {
        outputFile->Close();
        delete outputFile;
    }
    // Delete the input chain.
    if (inputChain) {
        delete inputChain;
    }
}

// Apply the configuration for a given run.
// Loads the input chain and creates the output file.
// Returns true if both steps succeed.
bool FileManager::ApplyConfig(int _run) {
    run = _run;
    inputChain = LoadTChain(run);
    if (!inputChain) {
        std::cerr << "Error: Failed to load input chain for run " << run << std::endl;
        return false;
    }
    outputFile = CreateOutputFile(run, 0); // Here we assume segment 0 for output.
    if (!outputFile || outputFile->IsZombie()) {
        std::cerr << "Error: Failed to create output file for run " << run << std::endl;
        return false;
    }
    return true;
}

TChain* FileManager::GetInputChain() const {
    return inputChain;
}

TFile* FileManager::GetOutputFile() const {
    return outputFile;
}

// Create a TChain by loading input ROOT files for the given run.
TChain* FileManager::LoadTChain(int run) {
    TChain* chain = new TChain(fileConfig.inputTree.c_str());
    segments = fileConfig.inputSegments;

    // If the configuration indicates "all" segments via a sentinel (-1),
    // detect segments dynamically.
    if (segments.size() == 1 && segments[0] == -1) {
        segments = DetectSegments(run);
    }

    bool loadedAtLeastOne = false;
    for (int seg : segments) {
        std::string filename = GetInputFilePath(run, seg);
        std::cout << "Loading file: " << filename << std::endl;
        TFile* file = TFile::Open(filename.c_str());
        if (file && !file->IsZombie()) {
            chain->Add(filename.c_str());
            std::cout << "Loaded segment " << seg << " with " 
                      << chain->GetEntries() << " entries." << std::endl;
            loadedAtLeastOne = true;
            file->Close();
        } else {
            std::cerr << "Warning: File does not exist or is corrupted: " << filename << std::endl;
        }
    }
    if (!loadedAtLeastOne) {
        delete chain;
        return nullptr;
    }
    return chain;
}

// Create an output TFile for the given run and segment.
TFile* FileManager::CreateOutputFile(int run, int segment) {
    std::string rootfilePath = GetOutputFilePath(run, segment);
    std::cout << "Creating output file: " << rootfilePath << std::endl;
    TFile* file = new TFile(rootfilePath.c_str(), "recreate");
    return file;
}

// Helper: Replace <run> and <segment> placeholders in a pattern.
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
        if (segments.size() == 1 && !segments[0] == -1) {
            result.replace(pos, 9, std::to_string(segment));
        } else {
            std::cerr << "Including <segment> in output pattern for multiple segment replay is ambiguous.\n";
        }
    }
    return fileConfig.basePath + fileConfig.outputSubdir + result;
}

// Detect segments using filesystem and regex based on the input pattern.
std::vector<int> FileManager::DetectSegments(int run) const {
    std::vector<int> segments;
    fs::path inputDir = fs::path(fileConfig.basePath) / fileConfig.inputSubdir;

    // Escape regex metacharacters outside of placeholders.
    std::string escapedPattern = EscapeRegexExceptPlaceholders(fileConfig.inputPattern);

    // Replace <run> with the actual run number.
    escapedPattern = std::regex_replace(escapedPattern, std::regex("<run>"), std::to_string(run));
    // Replace <segment> with a capture group matching one or two digits.
    escapedPattern = std::regex_replace(escapedPattern, std::regex("<segment>"), R"((\d{1,2}))");

    std::regex fileRegex(escapedPattern);

    for (const auto& entry : fs::directory_iterator(inputDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
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

// Escape regex special characters in the pattern, except within placeholders (<...>).
std::string FileManager::EscapeRegexExceptPlaceholders(const std::string& pattern) const {
    std::string escaped;
    bool inPlaceholder = false;
    for (char c : pattern) {
        if (c == '<') {
            inPlaceholder = true;
        }
        // If not inside a placeholder and character is a regex metacharacter, escape it.
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
