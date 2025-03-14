#ifndef REFERENCE_MANAGER_HPP
#define REFERENCE_MANAGER_HPP

#include "ManagerConfigs.hpp"
#include <memory>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include "TF1.h"
#include "Math/Interpolator.h"

class ReferenceManager {
public:
    // Now the constructor takes the run number and a ReferenceConfig (and/or GlobalConfig if needed).
    ReferenceManager(int run, const GlobalConfig& globalCfg, const ReferenceConfig& refCfg);

    bool LoadReferenceWaveforms();
    const ROOT::Math::Interpolator* GetInterpolator(int block) const;
    TF1* GetFitter(int block) const;
private:
    int run;
    int nblocks;
    int ntime;
    int timerefacc;
    const ReferenceConfig& refConfig;

    // Maps to store interpolators and fitters.
    std::map<int, std::unique_ptr<ROOT::Math::Interpolator>> interpolators;
    std::map<int, std::unique_ptr<TF1>> fitters;
};

#endif // REFERENCE_MANAGER_HPP
