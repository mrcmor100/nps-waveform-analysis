#ifndef WAVEFORM_FITTER_H
#define WAVEFORM_FITTER_H

#include <TF1.h>
#include <vector>
#include <memory>
#include "Math/Interpolator.h"
#include <algorithm>
#include <cassert>
#include <string>
#include "DataTypes.hpp"

class WaveformFitter {
public:
    using InterpolatorPtr = const ROOT::Math::Interpolator*;

    WaveformFitter(InterpolatorPtr interpolator,
                   double interpolationXMin = 1.0,
                   double interpolationXMax = 109.0);

    TF1* CreateTF1ForBlock(const BlockFitParameters& fitParams) const;

private:
    InterpolatorPtr interpolator;
    double xmin_, xmax_;
};

#endif // WAVEFORM_FITTER_H
