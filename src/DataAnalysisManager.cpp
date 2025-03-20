#include "DataAnalysisManager.hpp"
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RVec.hxx>
#include <TMath.h>
#include <iostream>

DataAnalysisManager::DataAnalysisManager(TChain* chain, const FileIOConfig& fileCfg, int run)
    : chain(chain), fileConfig(fileCfg), run(run)
{
}

void DataAnalysisManager::ProcessData() {
    ROOT::EnableImplicitMT(15);
    // Create the initial RDataFrame from the TChain.
    ROOT::RDataFrame df(*chain);

    // Step 1: Filter events based on tracking cuts.
    // This lambda filters out events with H.gtr.th, H.gtr.ph, and H.gtr.dp
    auto dfFiltered = df.Filter(
        [](double th, double ph, double dp) {
            return TMath::Abs(th) < 0.08 && TMath::Abs(ph) < 0.04 && TMath::Abs(dp) < 10;
        },
        {"H.gtr.th", "H.gtr.ph", "H.gtr.dp"}
    );

    // Step 2: Convert the raw waveform data from double to float.
    // We define a new branch "waveform_block_data" that converts the raw data.
    auto dfWaveform = dfFiltered.Define("waveform_block_data_float",
        [](const ROOT::RVec<double>& waveform, int nsamp) -> std::vector<float> {
            std::vector<float> flat_data;
            flat_data.reserve(nsamp);  // Avoid zero initialization
            std::transform(waveform.begin(), waveform.end(), std::back_inserter(flat_data),
                           [](double x) { return static_cast<float>(x); });
            return flat_data;
        },
        {"NPS.cal.fly.adcSampWaveform", "Ndata.NPS.cal.fly.adcSampWaveform"}
    );

    // Step 3: Flatten waveforms into block data.
    // Here you could, for example, take the flattened data and repackage it
    // into a vector of your DataBlockFloat structures (if desired).
    auto dfFlattened = dfWaveform.Define("blocks_float",
        [](const std::vector<float>& flat_data, int /*nsamp*/) -> std::vector<DataBlockFloat> {
            int nBlocks = flat_data.size() / DataBlockSize;
            std::vector<DataBlockFloat> blocks;
            blocks.reserve(nBlocks);
            for (int i = 0; i < nBlocks; ++i) {
                size_t offset = i * DataBlockSize;
                float block_id = flat_data[offset];
                float n_samples = flat_data[offset + 1];  // Must be <= NumSamples
                size_t num_to_copy = std::min<size_t>(n_samples, NumSamples); // Prevent out-of-bounds
    
                const float* waveform_ptr = &flat_data[offset + 2];  
                blocks.emplace_back(block_id, n_samples, waveform_ptr, num_to_copy);
            }
            return blocks;
        },
        {"waveform_block_data_float", "Ndata.NPS.cal.fly.adcSampWaveform"}
    );

    // Step 4: Process ADC counters.
    // Here you would encapsulate the ADC logic (like computing HMS time corrections,
    // updating pulse counts, and selecting pulse parameters) into a lambda.
    auto dfAdc = dfFlattened.Define("adc_results",
        [](int NadcCounters,
           const ROOT::RVec<double>& adcCounters,
           const ROOT::RVec<double>& adcSampPulseTime,
           const ROOT::RVec<double>& adcSampPulseTimeRaw,
           const ROOT::RVec<double>& adcSampPulseAmp,
           const ROOT::RVec<double>& adcSampPulseInt,
           const ROOT::RVec<double>& adcSampPulsePed) -> std::vector<AdcResult> 
        {
            // TODO: Implement slot-by-slot tdcoffset and timemean2 (if needed)
            Float_t tdcoffset = 0.; Float_t timemean2 = 150.;
            Float_t adcSampleDivisions = 16.0;
            Double_t corr_time_HMS = 0.0;
            // Resize adcResults to known size to avoid allocations
            std::vector<AdcResult> adcResults;
            adcResults.resize(NadcCounters);

            bool first = true;
            for (int i=0; i < NadcCounters; ++i) {
                if(first==true) {
                    corr_time_HMS = adcSampPulseTime[i] - 
                    (adcSampPulseTimeRaw[i] / adcSampleDivisions) - 
                    tdcoffset;
                }
                first = false;
                double expected = adcSampPulseTime[i] - 
                    (adcSampPulseTimeRaw[i] / adcSampleDivisions) - 
                    tdcoffset;
                
                AdcResult &res = adcResults[i];
                res.Npulse += 1;
                if (res.Npulse == 1) {
                    res.Sampampl = static_cast<float>(adcSampPulseAmp[i]);
                    res.Samptime = static_cast<float>(adcSampPulseTime[i]);
                    res.Sampener = static_cast<float>(adcSampPulseInt[i]);
                    res.Sampped  = static_cast<float>(adcSampPulsePed[i]);
                } else {
                    if (std::fabs(res.Samptime - timemean2) > std::fabs(adcSampPulseTime[i] - timemean2)) {
                        res.Sampampl = static_cast<float>(adcSampPulseAmp[i]);
                        res.Samptime = static_cast<float>(adcSampPulseTime[i]);
                        res.Sampener = static_cast<float>(adcSampPulseInt[i]);
                        res.Sampped  = static_cast<float>(adcSampPulsePed[i]);
                    }
                }
            }
            return adcResults;
        },
        {"Ndata.NPS.cal.fly.adcCounter",
         "NPS.cal.fly.adcCounter",
         "NPS.cal.fly.adcSampPulseTime",
         "NPS.cal.fly.adcSampPulseTimeRaw",
         "NPS.cal.fly.adcSampPulseAmp",
         "NPS.cal.fly.adcSampPulseInt",
         "NPS.cal.fly.adcSampPed"}
    );

    // Step 5: Compute additional quantities (energies, noise, signal widths, etc.)
    // Here, define another lambda that uses the previous branches.
    // auto dfFinal = dfAdc.Define("additional_quantities", 
    //     [](const ROOT::RVec<double>& waveform, int nsamp, /* other dependencies */) -> /* appropriate return type */ {
    //         // Implement energy, noise, width calculations here.
    //         // This example just returns nsamp for demonstration.
    //         return nsamp;
    //     },
    //     {"NPS.cal.fly.adcSampWaveform", "nblocks"} // list all dependencies required
    // );

    // Define the final output branches.
    std::vector<std::string> outputBranches = {
        "H.gtr.th",
        "H.gtr.ph",
        "H.gtr.dp",
        "adc_results"
    };

    std::string outputPath = fileConfig.basePath + fileConfig.outputSubdir + "flattened_output.root";
    std::cout << "Writing flattened output to: " << outputPath << std::endl;

    // Write the final TTree to disk. Note: Snapshot is the terminal action.
    dfAdc.Snapshot(fileConfig.outputTree, outputPath, outputBranches);
}
