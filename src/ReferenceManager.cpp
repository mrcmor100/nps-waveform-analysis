#include "ReferenceManager.hpp"

ReferenceManager::ReferenceManager(ConfigManager& configManager)
    : config(configManager) {
    run = config.GetInt("run");
    nblocks = config.GetInt("nblocks");
    ntime = config.GetInt("ntime");
}

bool ReferenceManager::ValidateWaveformFile(const std::string& filePath) {
    if (!fs::exists(filePath)) {
        std::cerr << "Error: File does not exist: " << filePath << std::endl;
        return false;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file: " << filePath << std::endl;
        return false;
    }

    double firstValue;
    file >> firstValue;
    if (file.fail()) {
        std::cerr << "Error: File format is incorrect: " << filePath << std::endl;
        return false;
    }

    return true;
}

bool ReferenceManager::LoadReferenceWaveforms() {
    std::cout << "Loading reference waveforms for run: " << run << std::endl;

    for (int i = 0; i < nblocks; i++) {
        std::string filePath = config.GetWaveformFile(run, i);

        if (!ValidateWaveformFile(filePath)) {
            std::cerr << "Skipping block " << i << " due to missing or invalid file." << std::endl;
            continue;
        }

        std::ifstream file(filePath);
        double timeRef, dum1;
        double x[ntime], y[ntime];

        file >> timeRef >> dum1;  // Read first two values
        for (int j = 0; j < ntime; j++) {
            file >> x[j] >> y[j];
        }

        auto interpolator = std::make_unique<ROOT::Math::Interpolator>(ntime, ROOT::Math::Interpolation::kCSPLINE);
        interpolator->SetData(ntime, x, y);
        interpolators[i] = std::move(interpolator);

        auto fitter = std::make_unique<TF1>(Form("finter_%d", i), "gaus", -200, ntime + 200);
        fitter->FixParameter(0, i);
        fitter->SetLineColor(4);
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
