#include <stdexcept>
#include <iostream>
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
    int   Npulse   = 0;
  };