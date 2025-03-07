#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <Math/Interpolator.h>
#include <TF1.h>
#include "ConfigManager.hpp"

namespace fs = std::filesystem;

class ReferenceManager {
public:
    explicit ReferenceManager(ConfigManager& configManager);
    
    bool LoadReferenceWaveforms();  // Loads all reference waveforms at initialization
    bool ValidateWaveformFile(const std::string& filePath);  // File format check
    const ROOT::Math::Interpolator* GetInterpolator(int block) const;
    TF1* GetFitter(int block) const;

private:
    ConfigManager& config;
    int run;
    int nblocks;
    int ntime;
    int maxwfpulses;
    std::unordered_map<int, std::unique_ptr<ROOT::Math::Interpolator>> interpolators;
    std::unordered_map<int, std::unique_ptr<TF1>> fitters;
};
