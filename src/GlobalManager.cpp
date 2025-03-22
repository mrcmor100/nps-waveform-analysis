#include "GlobalManager.hpp"

GlobalManager::GlobalManager(const GlobalConfig& globalCfg)
    : config(globalCfg), run(-1)
{
}

int GlobalManager::GetNblocks() const {
    return config.nblocks;
}

int GlobalManager::GetNtime() const {
    return config.ntime;
}

int GlobalManager::GetNchannel() const {
    return config.nchannel;
}

int GlobalManager::GetNcol() const {
    return config.ncol;
}

double GlobalManager::GetPeakTolerance() const {
    return config.peakTolerance;
}

void GlobalManager::ApplyConfig(int _run) {
    // Start with baseline settings already loaded in globalConfig.
    GlobalConfig internalConfig = config;

    // Check for any run-specific overrides.
    for (const auto& over : config.runOverrides) {
        if (run >= over.runLow && run <= over.runHigh) {
            if (over.ntimeOverride)
                internalConfig.ntime = over.ntimeOverride;
            if (over.timerefaccOverride)
                internalConfig.timerefacc = over.timerefaccOverride;
            // ... apply other overrides as needed.
        }
    }
    run = _run;
    config = internalConfig;
}