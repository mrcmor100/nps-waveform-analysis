#include "ReferenceManager.hpp"

ReferenceManager::ReferenceManager(ConfigManager& configManager) : config(configManager) {}

std::string ReferenceManager::GetReferenceWaveform(int run, int block) {
    return config.GetWaveformFile(run, block);
}
