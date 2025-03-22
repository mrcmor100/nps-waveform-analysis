#ifndef REFERENCE_MANAGER_HPP
#define REFERENCE_MANAGER_HPP

#include "ManagerConfigs.hpp"
#include <memory>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <set>
#include "TF1.h"
#include "Math/Interpolator.h"

class ReferenceManager {
public:
    // Now the constructor takes the run number and a ReferenceConfig (and/or GlobalConfig if needed).
    ReferenceManager(const ReferenceConfig& refCfg);

    const ROOT::Math::Interpolator* GetInterpolator(int block) const;
    TF1* GetFitter(int block) const;
    void ApplyConfig(int _run);
private:
    RunType runType;
    int run;
    const ReferenceConfig& refConfig;
    bool LoadReferenceWaveforms();
    // Maps to store interpolators and fitters.
    std::map<int, std::unique_ptr<ROOT::Math::Interpolator>> interpolators;
    std::map<int, std::unique_ptr<TF1>> fitters;
};

#endif // REFERENCE_MANAGER_HPP
