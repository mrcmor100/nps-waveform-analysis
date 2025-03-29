#ifndef DATATYPES_HPP
#define DATATYPES_HPP

#include <stdexcept>
#include <iostream>
#include <array>
#include "TH1.h"
#include "Config.hpp"

template <typename T, size_t N>
struct DataBlock {
    T block_id;
    T n_samples;
    T data[N];
    T errors[N];

    // Default constructor: needed for vector default construction.
    DataBlock() : block_id(0), n_samples(0) {
        // Optionally initialize data to 0.
        for (size_t i = 0; i < N; ++i) {
            data[i] = T();
            errors[i] = T();
        }
    }

    DataBlock(T id, T samples, const T* input_data, size_t num_to_copy, bool set_errors = false)
             : block_id(id), n_samples(samples) {
        size_t safe_copy_size = std::min(num_to_copy, N);  // Prevent out-of-bounds
        std::memcpy(data, input_data, safe_copy_size * sizeof(T));  
        // Zero out the rest if necessary
        if (safe_copy_size < N) {
            std::cout << "Should Never be called!\n";
            std::fill(data + safe_copy_size, data + N, T(0));
        }
        if (set_errors) {
            SetErrors();
        }
    }

    void SetErrors() {
        constexpr T const_error = std::abs(static_cast<T>(1.0) * static_cast<T>(4.096));
        for (size_t i = 0; i < N; ++i) {
            T val = data[i];
            errors[i] = (val < static_cast<T>(1.0)) ? const_error : std::abs(val * 4.096) / 4.096;
        }
    }
};
using DataBlockFloat = DataBlock<float, NumSamples>;
using DataBlockDouble = DataBlock<double, NumSamples>;

struct AdcResult {
    float Sampampl = -1;
    float Samptime = -1;
    float Sampener = -1;
    float Sampped = -1;
  };

struct AdcEventData {
    double HMS_corr_time = 0.0;
    int Npulse = 0;
    std::vector<AdcResult> adcResults;
};

// A single peak, storing amplitude and time
struct Peak {
    float amplitude = 0;
    float time = 0; 
    bool good = false;
};

// Container to store up to 12 peaks per waveform block.
struct PeakContainer {
    static constexpr int maxPeaks = 12; // Should be set by global configuration
    int block = 0;
    int nPeaks = 0;                      // Actual number of peaks found
    int peakOverflow = 0;
    std::array<Peak, maxPeaks> peaks{};  // Storage for peaks
};

// Structure to hold a single fit parameter and its limits
struct PeakFitParameter {
    double amplitude = -1.;
    double amplitude_lower_limit = -1.;
    double amplitude_upper_limit = -1.;
    double time = -1.;
    double time_lower_limit = -1.;
    double time_upper_limit = -1.;
};

struct PedestalFitParameter {
    double pedestal = -50.;
    double ped_lower_limit = -60.;
    double ped_upper_limit = -40.;
};

struct BlockFitParameters {
    // Parameters indexed from 1 up to 2*maxPulses (for peaks)
    // plus parameter 1 which is a baseline / pedestal
    PedestalFitParameter block_pedestal; // Fit limits for baseline
    // Maybe I shouldn't use PeakContainer::maxPeaks size?
    std::array<PeakFitParameter,PeakContainer::maxPeaks> peak_parameters;
    // Good flag indicating whether the "good" peak condition was met.
    int nPeaks = 0.;
    int block = 0;
    bool good = false;
};

struct PeakFitResults {
    double time = -999.;
    double time_err = -1.;
    double amplitude = -999.;
    double amplitude_err = -1.;
};

struct PedestalFitResults {
    double pedestal = -999.;
    double pedestal_err = -1.;
};

struct BlockFitResults {
    PedestalFitResults pedestal;               // Fit result for baseline
    std::vector<PeakFitResults> peaks;         // One per peak in fit
    double chi2 = -1.;                         // Total chi-squared from TF1
    int ndf = -1;                              // Degrees of freedom
    bool good = false;                         // Fit passed quality threshold
};

#endif