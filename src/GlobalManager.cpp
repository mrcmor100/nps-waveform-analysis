#include "GlobalManager.hpp"

GlobalManager::GlobalManager(const GlobalConfig& globalCfg)
    : config(globalCfg)
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
