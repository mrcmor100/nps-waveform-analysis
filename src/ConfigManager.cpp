#include "ConfigManager.hpp"

ConfigManager::ConfigManager(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + configPath);
    }
    file >> config;
}

int ConfigManager::GetInt(const std::string& key) {
    return config["global"].contains(key) ? config["global"][key].get<int>() : -1;
}

double ConfigManager::GetDouble(const std::string& key) {
    return config["global"].contains(key) ? config["global"][key].get<double>() : -1.0;
}

std::string ConfigManager::GetString(const std::string& key) {
    return config.contains(key) ? config[key].get<std::string>() : "";
}

std::vector<int> ConfigManager::GetSegments() {
    json segmentsConfig = config["processing_settings"]["segments"];

    std::vector<int> segments;

    if (segmentsConfig.is_string() && segmentsConfig.get<std::string>() == "all") {
        for (int i = 0; i < 10; i++) {  // Adjust max segment count
            segments.push_back(i);
        }
    } else if (segmentsConfig.is_number()) {
        segments.push_back(segmentsConfig.get<int>());
    } else if (segmentsConfig.is_array()) {
        segments = segmentsConfig.get<std::vector<int>>();
    } else if (segmentsConfig.is_object() && segmentsConfig.contains("range")) {
        int start = segmentsConfig["range"][0];
        int end = segmentsConfig["range"][1];
        for (int i = start; i <= end; i++) {
            segments.push_back(i);
        }
    } else {
        std::cerr << "Error: Invalid segments configuration in JSON." << std::endl;
    }

    return segments;
}

std::string ConfigManager::ResolvePath(const std::string& pattern, int value1, int value2) {
    std::string base_path = config["project_settings"]["base_path"].get<std::string>();
    std::string fullPath = base_path + pattern;
    if (value1 >= 0 && value2 >= 0) return Form(fullPath.c_str(), value1, value2);
    else if (value1 >= 0) return Form(fullPath.c_str(), value1);
    return fullPath;
}

std::string ConfigManager::GetFilePath(const std::string& key, int run, int segment) {
    return ResolvePath(config["filename_patterns"][key].get<std::string>(), run, segment);
}

std::string ConfigManager::GetWaveformFile(int run, int block_number) {
    for (const auto& wf : config["waveform_files"]) {
        int minRun = wf["range"][0].get<int>();
        int maxRun = wf["range"][1].get<int>();
        if (run >= minRun && run <= maxRun) {
            return ResolvePath(wf["path"], block_number);
        }
    }
    return "";
}

std::vector<std::string> ConfigManager::GetBranchNames() {
    std::vector<std::string> branchNames;
    for (const auto& branch : config["branches"]) {
        branchNames.push_back(branch["name"].get<std::string>());
    }
    return branchNames;
}

std::unordered_map<std::string, std::string> ConfigManager::GetBranchVariableMap() {
    std::unordered_map<std::string, std::string> branchVarMap;
    for (const auto& branch : config["branches"]) {
        branchVarMap[branch["name"].get<std::string>()] = branch["variable"].get<std::string>();
    }
    return branchVarMap;
}
