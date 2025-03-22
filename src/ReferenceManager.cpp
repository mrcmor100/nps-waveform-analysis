#include "ReferenceManager.hpp"
#include <filesystem>
namespace fs = std::filesystem;

ReferenceManager::ReferenceManager(const ReferenceConfig& refCfg)
    : refConfig(refCfg), run(-1)
{
}
    

bool ReferenceManager::LoadReferenceWaveforms() {
    std::cout << "Loading reference waveforms for run: " << run << std::endl;

    // Sort all patterns by descending rangeStart (recency)
    std::vector<const FilePattern*> sortedPatterns;
    for (const auto& p : refConfig.waveformPatterns)
        sortedPatterns.push_back(&p);
    
    std::sort(sortedPatterns.begin(), sortedPatterns.end(), [](const FilePattern* a, const FilePattern* b) {
        return std::max(a->rangeStart, a->elasticStart) > std::max(b->rangeStart, b->elasticStart);
    });

    // Build prioritized patternOrder: first those that match this run
    std::vector<const FilePattern*> patternOrder;
    std::set<const FilePattern*> seen;

    for (const auto* fp : sortedPatterns) {
        if (fp->MatchesProd(run) || fp->MatchesElastic(run)) {
            patternOrder.push_back(fp);
            seen.insert(fp);
        }
    }

    // Append remaining fallbacks
    for (const auto* fp : sortedPatterns) {
        // Patterns must for runs prior to this run.
        if (!seen.count(fp) && fp->rangeEnd < run && fp->elasticEnd < run) {
            patternOrder.push_back(fp);
        }
    }

    // Always add default as last fallback
    static const FilePattern defaultPattern = {0, 0, 0, 0, refConfig.waveformDefault};
    patternOrder.push_back(&defaultPattern);

    std::set<int> requiredChannels;
    for (const auto& ch : refConfig.ChannelTypes) {
        if(ch.channelName.compare("blocks") == 0) {
            std::cout << "Including all Block channels.\n";
            for (int i = ch.chStart; i <= ch.chEnd; ++i) {
                requiredChannels.insert(i);
            }
        } else {
            std::cout << "Excluding all: " << ch.channelName << " channels\n";
        }
    }

    std::set<int> loadedBlocks;
    std::vector<double> xbins;

    for (const FilePattern* pattern : patternOrder) {
        const std::string& filePath = pattern->path;
        if (!fs::exists(filePath)) {
            std::cerr << "Warning: waveform file missing: " << filePath << std::endl;
            continue;
        }

        std::ifstream infile(filePath);
        if (!infile.is_open()) {
            std::cerr << "Error opening: " << filePath << std::endl;
            continue;
        }

        std::string line;
        bool parsedBinning = false;

        while (std::getline(infile, line)) {
            if (line.empty() || line[0] == '#') {
                if (line.find("Bining of References:") != std::string::npos) {
                    std::getline(infile, line);  // Read the actual values line
                    std::istringstream binline(line);
                    double min, max;
                    int steps;
                    binline >> min >> max >> steps;
                    if (binline.fail()) {
                        std::cerr << "Error parsing binning from: " << filePath << std::endl;
                        return false;
                    }

                    xbins.resize(steps);
                    double stepSize = (max - min) / (steps - 1);
                    for (int i = 0; i < steps; ++i)
                        xbins[i] = min + i * stepSize;

                    parsedBinning = true;
                }
                continue;
            }

            if (!parsedBinning) {
                std::cerr << "Error: binning must be defined before waveform data in file: " << filePath << std::endl;
                return false;
            }

            std::istringstream iss(line);
            int block;
            double timeRef, tdcOffset;
            char comma;
            iss >> block >> timeRef >> tdcOffset;

            if (loadedBlocks.count(block)) continue;

            std::vector<double> y;
            double val;
            while (iss >> val) y.push_back(val);

            if (y.size() != xbins.size()) {
                std::cerr << "Mismatch: block " << block
                          << " has " << y.size()
                          << " values, expected " << xbins.size() << std::endl;
                continue;
            }

            auto interpolator = std::make_unique<ROOT::Math::Interpolator>(xbins.size(), ROOT::Math::Interpolation::kCSPLINE);
            interpolator->SetData(xbins.size(), xbins.data(), y.data());
            interpolators[block] = std::move(interpolator);

            auto fitter = std::make_unique<TF1>(("finter_" + std::to_string(block)).c_str(), "gaus", -200, 200);
            fitter->FixParameter(0, block);
            fitter->SetNpx(1100);
            fitters[block] = std::move(fitter);

            loadedBlocks.insert(block);
        }

        if (loadedBlocks.size() >= requiredChannels.size())
            break;
    }

    for (int ch : requiredChannels) {
        if (!interpolators.count(ch)) {
            std::cerr << "Missing waveform for required block: " << ch << std::endl;
        }
    }

    return loadedBlocks.size() == requiredChannels.size();
}


const ROOT::Math::Interpolator* ReferenceManager::GetInterpolator(int block) const {
    auto it = interpolators.find(block);
    return (it != interpolators.end()) ? it->second.get() : nullptr;
}

TF1* ReferenceManager::GetFitter(int block) const {
    auto it = fitters.find(block);
    return (it != fitters.end()) ? it->second.get() : nullptr;
}

void ReferenceManager::ApplyConfig(int _run) {
    run = _run;
    runType = RunType::Unknown;
    for (const auto& pattern : refConfig.waveformPatterns) {
        if (pattern.MatchesProd(run)) {
            runType = RunType::Prod;
            break;
        } else if (pattern.MatchesElastic(run)) {
            runType = RunType::Elastic;
            break;
        }
    }

    std::cout << "Run " << run << " is a "
              << (runType == RunType::Prod ? "production" :
                  runType == RunType::Elastic ? "elastic" : "unknown")
              << " run.\n";

    
    // Load reference waveforms.
    if (this->LoadReferenceWaveforms()) {
        std::cerr << "Failed to load reference waveforms.\n";
        //Throw runtime error?
    }
}
