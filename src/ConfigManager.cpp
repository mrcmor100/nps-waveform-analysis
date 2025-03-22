#include "ConfigManager.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <regex>

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
        globalConfig.peakTolerance = g.at("peak_tolerance").get<double>();
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
        fileIOConfig.outputTree    = io.at("output_tree").get<std::string>();

        if (fileIOConfig.inputPattern.find("<run>") == std::string::npos) {
            throw std::runtime_error("input_rootfile pattern must contain at least the <run> keyword.");
        }

        // Flexible input_segments: can be an array, a range like "2-5", or "all".
        const auto& segments = io.at("input_segments");
        if (segments.is_array()) {
            for (const auto& seg : segments) {
                fileIOConfig.inputSegments.push_back(seg.get<int>());
            }
        } else if (segments.is_string()) {
            std::string segStr = segments.get<std::string>();
            static const std::regex rangePattern(R"(^\d+-\d+$)");
        
            if (segStr == "all") {
                fileIOConfig.inputSegments = {-1};  // sentinel for "all"
            } else if (std::regex_match(segStr, rangePattern)) {
                size_t dash = segStr.find('-');
                int start = std::stoi(segStr.substr(0, dash));
                int end   = std::stoi(segStr.substr(dash + 1));
        
                if (start > end) {
                    std::cerr << "Warning: input_segments range start > end: " << segStr << "\n";
                } else {
                    for (int i = start; i <= end; ++i) {
                        fileIOConfig.inputSegments.push_back(i);
                    }
                }
            } else {
                std::cerr << "Invalid format for input_segments: \"" << segStr
                          << "\". Must be \"all\", a list, or a simple range like \"2-5\".\n";
            }
        }
    } catch (const std::exception &e) {
        std::cerr << "Error loading file I/O configuration: " << e.what() << std::endl;
    }
}


void ConfigManager::LoadReferenceConfig() {
    try {
        auto& wfFiles = config.at("waveform_files");

        std::vector<std::pair<int, int>> allProdRanges;
        std::vector<std::pair<int, int>> allElasticRanges;

        for (const auto& pat : wfFiles.at("patterns")) {
            FilePattern fp;
            fp.rangeStart   = pat.at("prod_range")[0].get<int>();
            fp.rangeEnd     = pat.at("prod_range")[1].get<int>();
            fp.elasticStart = pat.at("elastic_range")[0].get<int>();
            fp.elasticEnd   = pat.at("elastic_range")[1].get<int>();
            fp.path         = pat.at("path").get<std::string>();

            referenceConfig.waveformPatterns.push_back(fp);

            allProdRanges.emplace_back(fp.rangeStart, fp.rangeEnd);
            allElasticRanges.emplace_back(fp.elasticStart, fp.elasticEnd);
        }

        // Check for cross-pattern overlap between prod and elastic ranges
        for (const auto& [pStart, pEnd] : allProdRanges) {
            for (const auto& [eStart, eEnd] : allElasticRanges) {
                if (std::max(pStart, eStart) <= std::min(pEnd, eEnd)) {
                    std::cerr << "Error: Overlap detected between prod range ["
                              << pStart << "," << pEnd << "] and elastic range ["
                              << eStart << "," << eEnd << "]\n";
                    throw std::runtime_error("Overlapping production and elastic ranges across patterns.");
                }
            }
        }

        referenceConfig.waveformDefault = wfFiles.at("default_pattern").get<std::string>();

        for (const auto& pat : wfFiles.at("channels")) {
            ChannelType ct;
            ct.channelName = pat.at("name").get<std::string>();
            ct.chStart     = pat.at("range")[0].get<int>();
            ct.chEnd       = pat.at("range")[1].get<int>();
            referenceConfig.ChannelTypes.push_back(ct);
        }
    } catch (const std::exception& e) {
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
