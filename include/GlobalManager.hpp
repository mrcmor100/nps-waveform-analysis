#ifndef GLOBAL_MANAGER_HPP
#define GLOBAL_MANAGER_HPP

#include "ManagerConfigs.hpp"

class GlobalManager {
public:
    // Accepts a GlobalConfig struct at construction.
    GlobalManager(const GlobalConfig& globalCfg);

    // Accessor methods for global parameters.
    int GetNblocks() const;
    int GetNtime() const;
    int GetNchannel() const;
    int GetNcol() const;
    double GetPeakTolerance() const;
    void ApplyConfig(int run); 
    // You can add additional getters as needed.
private:
    int run;
    GlobalConfig config;
};

#endif // GLOBAL_MANAGER_HPP
