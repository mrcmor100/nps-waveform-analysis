#include <stdexcept>
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

    DataBlock(T id, T samples, const T* input_data) : block_id(id), n_samples(samples) {
        if (n_samples != N) {
            throw std::invalid_argument("DataBlock constructed with incorrect n_samples");
        }
        std::memcpy(data, input_data, N * sizeof(T));  // Efficient copy
    }
};
using DataBlockFloat = DataBlock<float, NumSamples>;
using DataBlockDouble = DataBlock<double, NumSamples>;