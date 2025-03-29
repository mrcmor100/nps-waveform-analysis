// WaveformFitter.cpp
#include "WaveformFitter.hpp"

WaveformFitter::WaveformFitter(InterpolatorPtr interpolator,
                               double interpolationXMin,
                               double interpolationXMax)
    : interpolator(interpolator),
    xmin_(interpolationXMin),
    xmax_(interpolationXMax)
{}

TF1* WaveformFitter::CreateTF1ForBlock(const BlockFitParameters& fitParams) const
{
    const int nPeaks = fitParams.nPeaks;
    const int blockId = fitParams.block;

    const int numParams = 1 + 2 * nPeaks; // 1 for pedestal + 2 per peak

    auto fitFunction = [interpolator_ = interpolator, nPeaks, xmin = xmin_, xmax = xmax_]
                       (double* x, double* parameters) -> double
    {
        const double constantOffset = parameters[0];
        double sum = 0.0;

        for (int pulseIndex = 0; pulseIndex < nPeaks; ++pulseIndex)
        {
            const int timeParamIndex = 1 + 2 * pulseIndex;
            const int amplitudeParamIndex = 2 + 2 * pulseIndex;

            const double timeShift = parameters[timeParamIndex];
            const double amplitude = parameters[amplitudeParamIndex];

            const double shiftedTime = x[0] - timeShift;
            const double inDomain = (shiftedTime > xmin && shiftedTime < xmax);
            const double safeTime = std::clamp(shiftedTime, xmin, xmax);

            sum += inDomain * amplitude * interpolator_->Eval(safeTime);
        }

        return sum + constantOffset;
    };

    auto tf1 = new TF1("", fitFunction, 0, 150, numParams); // Adjust range as needed

    // Set pedestal
    tf1->SetParameter(0, fitParams.block_pedestal.pedestal);
    tf1->SetParLimits(0,
                     fitParams.block_pedestal.ped_lower_limit,
                     fitParams.block_pedestal.ped_upper_limit);

    for (int i = 0; i < nPeaks; ++i)
    {
        const auto& peak = fitParams.peak_parameters[i];
        const int timeIndex = 1 + 2 * i;
        const int ampIndex  = 2 + 2 * i;

        tf1->SetParameter(timeIndex, peak.time);
        tf1->SetParLimits(timeIndex, peak.time_lower_limit, peak.time_upper_limit);

        tf1->SetParameter(ampIndex, peak.amplitude);
        tf1->SetParLimits(ampIndex, peak.amplitude_lower_limit, peak.amplitude_upper_limit);
    }

    return tf1;
}
