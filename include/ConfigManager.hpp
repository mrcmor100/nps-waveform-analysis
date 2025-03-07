/**
 * @file ConfigManager.hpp
 * @brief Handles configuration loading from JSON.
 */

 #pragma once
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

#include "TString.h"

using json = nlohmann::json;

/**
 * @class ConfigManager
 * @brief Manages project configuration using JSON.
 */
class ConfigManager {
public:
    /**
     * @brief Loads configuration from a JSON file.
     * @param configPath Path to the JSON file.
     */
    explicit ConfigManager(const std::string& configPath);
    
    /**
     * @brief Retrieves an integer parameter from global settings.
     * @param key The name of the parameter.
     * @return The integer value.
     */
    int GetInt(const std::string& key);
    double GetDouble(const std::string& key);
    std::string GetString(const std::string& key);
    std::vector<int> GetSegments();
    /**
     * @brief Resolves a file path using predefined patterns.
     * @param key The name of the file type.
     * @param run Optional run number.
     * @return The resolved file path.
     */
    std::string GetFilePath(const std::string& key, int run = -1, int segment = -1);
    std::vector<std::string> GetBranchNames();
    std::unordered_map<std::string, std::string> GetBranchVariableMap();
    std::string GetWaveformFile(int run, int block_number);

private:
    json config;
    std::string ResolvePath(const std::string& pattern, int value1 = -1, int value2 = -1);
};
