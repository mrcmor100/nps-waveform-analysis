#include "ReferenceManager.hpp"
#include <filesystem>
namespace fs = std::filesystem;

ReferenceManager::ReferenceManager(int run, const GlobalConfig& globalCfg, const ReferenceConfig& refCfg)
    : run(run), nblocks(globalCfg.nblocks), ntime(globalCfg.ntime), refConfig(refCfg)
{
}

bool ReferenceManager::LoadReferenceWaveforms() {
    std::cout << "Loading reference waveforms for run: " << run << std::endl;
    // Loop over blocks; here we assume block indices 0 to nblocks-1.
    for (int i = 0; i < nblocks; i++) {
        // Determine the waveform file based on run.
        std::string filePath;
        bool matched = false;
        for (const auto& fp : refConfig.waveformPatterns) {
            if (run >= fp.rangeStart && run <= fp.rangeEnd) {
                // For demonstration, assume filePath is formatted with the block number.
                char buffer[256];
                snprintf(buffer, sizeof(buffer), fp.path.c_str(), i);
                filePath = buffer;
                matched = true;
                break;
            }
        }
        if (!matched) {
            // Use default pattern.
            char buffer[256];
            snprintf(buffer, sizeof(buffer), refConfig.waveformDefault.c_str(), i);
            filePath = buffer;
        }
        
        // Validate file existence.
        if (!fs::exists(filePath)) {
            std::cerr << "Error: File does not exist: " << filePath << std::endl;
            continue;
        }
        
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Error: Unable to open file: " << filePath << std::endl;
            continue;
        }
        
        // Read file header values.
        double timeRef, dummy;
        file >> timeRef >> dummy;
        if (file.fail()) {
            std::cerr << "Error: File format is incorrect: " << filePath << std::endl;
            continue;
        }
        
        double x[ntime], y[ntime];
        for (int j = 0; j < ntime; j++) {
            file >> x[j] >> y[j];
            if (file.fail()) {
                std::cerr << "Error reading waveform data from: " << filePath << std::endl;
                break;
            }
        }
        
        auto interpolator = std::make_unique<ROOT::Math::Interpolator>(ntime, ROOT::Math::Interpolation::kCSPLINE);
        interpolator->SetData(ntime, x, y);
        interpolators[i] = std::move(interpolator);
        
        auto fitter = std::make_unique<TF1>(("finter_" + std::to_string(i)).c_str(), "gaus", -200, ntime + 200);
        fitter->FixParameter(0, i);
        fitter->SetNpx(1100);
        fitters[i] = std::move(fitter);
    }
    return true;
}

const ROOT::Math::Interpolator* ReferenceManager::GetInterpolator(int block) const {
    auto it = interpolators.find(block);
    return (it != interpolators.end()) ? it->second.get() : nullptr;
}

TF1* ReferenceManager::GetFitter(int block) const {
    auto it = fitters.find(block);
    return (it != fitters.end()) ? it->second.get() : nullptr;
}
