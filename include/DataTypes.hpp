#include <stdexcept>
#include <iostream>
#include <array>
#include "Config.hpp"

template <typename T, size_t N>
struct DataBlock {
    T block_id;
    T n_samples;
    T data[N];

    // Default constructor: needed for vector default construction.
    DataBlock() : block_id(0), n_samples(0) {
        // Optionally initialize data to 0.
        for (size_t i = 0; i < N; ++i)
            data[i] = T();
    }

    DataBlock(T id, T samples, const T* input_data, size_t num_to_copy) : block_id(id), n_samples(samples) {
        size_t safe_copy_size = std::min(num_to_copy, N);  // Prevent out-of-bounds
        std::memcpy(data, input_data, safe_copy_size * sizeof(T));  
        // Zero out the rest if necessary
        if (safe_copy_size < N) {
            std::cout << "Should Never be called!\n";
            std::fill(data + safe_copy_size, data + N, T(0));
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
    int block = 0;
    float amplitude = 0;
    float time = 0;
};

// Container to store up to 12 peaks per waveform block.
struct PeakContainer {
    static constexpr int maxPeaks = 12; // Should be set by global configuration
    int nPeaks = 0;                      // Actual number of peaks found
    int peakOverflow = 0;
    std::array<Peak, maxPeaks> peaks{};  // Storage for peaks
};

// Structure to hold a single fit parameter and its limits
struct FitParameter {
    double value;
    double lower_limit;
    double upper_limit;
};

struct FitResults {
    bool good;       // true if a good peak is found
    double time;     // selected pulse time (or fallback)
    double amplitude; // selected pulse amplitude (or fallback)
};

struct BlockFitParameters {
    // Parameters indexed from 1 up to 2*maxPulses (for peaks)
    // plus parameter 1 which is a baseline/pedestal
    std::vector<FitParameter> parameters;
    // Good flag indicating whether the "good" peak condition was met.
    bool good;
};