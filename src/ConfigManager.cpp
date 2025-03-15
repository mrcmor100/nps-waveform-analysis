#include "ConfigManager.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

ConfigManager::ConfigManager(const std::string& configPath, int run)
    : currentRun(run)
{
    LoadConfig(configPath);
    LoadGlobalConfig();
    LoadBranchConfig();
    LoadFileIOConfig();
    LoadReferenceConfig();
    ApplyRunOverrides();
}

void ConfigManager::LoadConfig(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + configPath);
    }
    file >> config;
}

void ConfigManager::LoadGlobalConfig() {
    try {
        auto& g = config.at("global");
        globalConfig.nchannel   = g.at("nchannel").get<int>();
        globalConfig.nblocks    = g.at("nblocks").get<int>();
        globalConfig.ncol       = g.at("ncol").get<int>();
        globalConfig.nlin       = g.at("nlin").get<int>();
        globalConfig.maxpulses  = g.at("maxpulses").get<int>();
        globalConfig.maxwfpulses= g.at("maxwfpulses").get<int>();
        globalConfig.ntemp      = g.at("ntemp").get<int>();
        globalConfig.ntime      = g.at("ntime").get<int>();
        globalConfig.nfADC      = g.at("nfADC").get<int>();
        globalConfig.dt         = g.at("dt").get<double>();
        globalConfig.nslots     = g.at("nslots").get<int>();
        globalConfig.ADCtomV    = g.at("ADCtomV").get<double>();
        globalConfig.integtopC  = g.at("integtopC").get<double>();
        globalConfig.timerefacc = g.at("timerefacc").get<double>();
    } catch (const std::exception &e) {
        throw std::runtime_error("Error loading global configuration: " + std::string(e.what()));
    }
}

void ConfigManager::LoadBranchConfig() {
    try {
        for (const auto& branch : config.at("branches")) {
            branchConfig.branchNames.push_back(branch.at("name").get<std::string>());
            branchConfig.branchVarMap[branch.at("name").get<std::string>()] = branch.at("variable").get<std::string>();
        }
    } catch (const std::exception &e) {
        std::cerr << "Error loading branch configuration: " << e.what() << std::endl;
    }
}

void ConfigManager::LoadFileIOConfig() {
    try {
        auto& io = config.at("io_settings");
        fileIOConfig.basePath      = io.at("base_path").get<std::string>();
        fileIOConfig.inputSubdir   = io.at("input_subdir").get<std::string>();
        fileIOConfig.outputSubdir  = io.at("output_subdir").get<std::string>();
        fileIOConfig.inputPattern  = io.at("input_rootfile").get<std::string>();
        fileIOConfig.outputPattern = io.at("output_rootfile").get<std::string>();
        fileIOConfig.inputTree     = io.at("input_tree").get<std::string>();
    } catch (const std::exception &e) {
        std::cerr << "Error loading file I/O configuration: " << e.what() << std::endl;
    }
}

void ConfigManager::LoadReferenceConfig() {
    try {
        // Load waveform file patterns.
        auto& wfFiles = config.at("waveform_files");
        for (const auto& pat : wfFiles.at("patterns")) {
            FilePattern fp;
            fp.rangeStart = pat.at("range")[0].get<int>();
            fp.rangeEnd   = pat.at("range")[1].get<int>();
            fp.path       = pat.at("path").get<std::string>();
            referenceConfig.waveformPatterns.push_back(fp);
        }
        referenceConfig.waveformDefault = wfFiles.at("default_pattern").get<std::string>();

        // Load tdc file patterns.
        auto& tdcFiles = config.at("tdc_files");
        for (const auto& pat : tdcFiles.at("patterns")) {
            FilePattern fp;
            fp.rangeStart = pat.at("range")[0].get<int>();
            fp.rangeEnd   = pat.at("range")[1].get<int>();
            fp.path       = pat.at("path").get<std::string>();
            referenceConfig.tdcPatterns.push_back(fp);
        }
        referenceConfig.tdcDefault = tdcFiles.at("default_pattern").get<std::string>();
    } catch (const std::exception &e) {
        std::cerr << "Error loading reference configuration: " << e.what() << std::endl;
    }
}

void ConfigManager::ApplyRunOverrides() {
    // run_parameters is an array of override objects.
    if (config.contains("run_parameters")) {
        for (const auto& overrideObj : config.at("run_parameters")) {
            // Ensure a "range" field exists.
            if (!overrideObj.contains("range") || !overrideObj.at("range").is_array()) {
                std::cerr << "Skipping invalid run_parameters override entry." << std::endl;
                continue;
            }
            int low = overrideObj.at("range")[0].get<int>();
            int high = overrideObj.at("range")[1].get<int>();
            if (currentRun < low || currentRun > high) {
                continue; // This override does not apply to the current run.
            }
            // For each key in the override (other than "range"), check if it belongs to global or reference.
            for (auto it = overrideObj.begin(); it != overrideObj.end(); ++it) {
                std::string key = it.key();
                if (key == "range")
                    continue;
                // If the key exists in global, override it.
                if (globalConfig.nblocks && key == "nblocks") {
                    globalConfig.nblocks = it.value().get<int>();
                }
                if (globalConfig.ntime && key == "ntime") {
                    globalConfig.ntime = it.value().get<int>();
                }
                if (globalConfig.ncol && key == "ncol") {
                    globalConfig.ncol = it.value().get<int>();
                }
                // If not in global, check if it belongs to ReferenceConfig.
                if (key == "timerefacc") {
                    globalConfig.timerefacc = it.value().get<int>();
                    std::cout << "Overriding timerefacc for run: " << currentRun << '\n';
                }
                // Add further keys as needed.
            }
        }
    }
}
