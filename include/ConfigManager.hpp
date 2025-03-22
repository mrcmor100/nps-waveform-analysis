#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include "ManagerConfigs.hpp"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class ConfigManager {
public:
    // The constructor now takes both the path and the run number.
    ConfigManager(const std::string& configPath, int run);

    // Getters for our configuration structs.
    const ApplicationConfig& GetApplicationConfig() const { return applicationConfig; }
    const GlobalConfig& GetGlobalConfig() const { return globalConfig; }
    const BranchConfig& GetBranchConfig() const { return branchConfig; }
    const FileIOConfig& GetFileIOConfig() const { return fileIOConfig; }
    const ReferenceConfig& GetReferenceConfig() const { return referenceConfig; }

    // (Other helper functions from before can be kept if still needed.)
private:
    // Load JSON file.
    void LoadConfig(const std::string& configPath);
    // Load sections into our structs.
    void LoadApplicationConfig();
    void LoadGlobalConfig();
    void LoadBranchConfig();
    void LoadFileIOConfig();
    void LoadReferenceConfig();
    // Apply run-specific overrides (using run_parameters).
    void ApplyRunOverrides();

    json config;
    int currentRun;

    ApplicationConfig applicationConfig;
    GlobalConfig globalConfig;
    BranchConfig branchConfig;
    FileIOConfig fileIOConfig;
    ReferenceConfig referenceConfig;
};

#endif // CONFIG_MANAGER_HPP
