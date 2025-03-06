#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "ConfigManager.hpp"

class ReferenceManager {
public:
    explicit ReferenceManager(ConfigManager& configManager);
    std::string GetReferenceWaveform(int run, int block);

private:
    ConfigManager& config;
};
