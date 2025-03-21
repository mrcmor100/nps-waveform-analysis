#include "DataAnalysisManager.hpp"
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RVec.hxx>
#include <TMath.h>
#include <iostream>
#include <algorithm>  // for std::min

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
    // We define a new branch "waveform_block_data_float" that converts the raw data.
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

    // Step 3a: Determine Number of Waveforms assuming DataBlockSized DataBlocks
    // Save the size for use in future steps.
    auto dfSizeBlocks = dfWaveform.Define("n_blocks",
        [] (int adcSampWaveformSize) {
            return adcSampWaveformSize / static_cast<int>(DataBlockSize);
        },
        {"Ndata.NPS.cal.fly.adcSampWaveform"}
    );

    // Step 3b: Make DataBlockFloat structures from raw float arrays.
    // Future steps can operate directly on DataBlockFloats within each event.
    auto dfFlattened = dfSizeBlocks.Define("blocks_float",
        [](const std::vector<float>& flat_data, int nBlocks) -> std::vector<DataBlockFloat> {
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
        {"waveform_block_data_float", "n_blocks"}
    );

    auto findPeaks = [](const DataBlockFloat &block) -> PeakContainer {
        PeakContainer result;
        // Loop over the data; note: adjust the upper bound if needed.
        for (int it = 0; it < NumSamples - 6; ++it) {
            // Example peak-finding criteria:
            // Check that the waveform increases from it+0 to it+2, then levels off,
            // and then decreases after it+3. Adjust these conditions to your needs.
            if (block.data[it] < block.data[it+1] &&
                block.data[it+1] < block.data[it+2] &&
                block.data[it+2] <= block.data[it+3] &&
                block.data[it+3] >= block.data[it+4] &&
                block.data[it+4] > 0) {
                // We found a peak; record its amplitude and time.
                // Here we assume the peak is at it+3; you might choose a more sophisticated
                // estimate of the peak time.
                if (result.nPeaks < PeakContainer::maxPeaks) {
                    result.peaks[result.nPeaks].amplitude = block.data[it+3];
                    result.peaks[result.nPeaks].time = static_cast<float>(it+3);
                    result.nPeaks++;
                } else {
                    result.peakOverflow = 1;
                    break;
                }
                // Skip a few bins to avoid double-counting this peak.
                it += 4;
            }
        }
        return result;
    };

    // Step 4: Find Peaks within each DataBlockFloat.Data()
    // Find all the peak amplitude and time within the individual DataBlockFloat objects.
    auto dfPeaks = dfFlattened.Define("block_peaks",
        [findPeaks](const std::vector<DataBlockFloat>& blocks) -> std::vector<PeakContainer> {
            std::vector<PeakContainer> peaks;
            peaks.reserve(blocks.size());
            for (const auto &block : blocks) {
                peaks.push_back(findPeaks(block));
            }
            return peaks;
        },
        {"blocks_float"}
    );    

    // Lambda to process a single ADC entry
    auto processAdcEntry = [](AdcResult& res,
        float pulseAmp, float pulseTime, float pulseInt,
        float pulsePed, float timemean2, int Npulse) {
        // If it's the first pulse or the new pulse is closer to the reference time, update the result.
        if (Npulse == 1 || std::fabs(res.Samptime - timemean2) > 
                               std::fabs(pulseTime    - timemean2)) {
            res.Sampampl = pulseAmp;
            res.Samptime = pulseTime;
            res.Sampener = pulseInt;
            res.Sampped  = pulsePed;
        }
    };

    // Step 5: Calculate ADC parameters.
    // Here we encapsulate the ADC logic (like computing HMS time corrections,
    auto dfAdc = dfPeaks.Define("adc_results",
        [processAdcEntry](int NadcCounters,
                           const ROOT::RVec<double>& adcCounters,
                           const ROOT::RVec<double>& adcSampPulseTime,
                           const ROOT::RVec<double>& adcSampPulseTimeRaw,
                           const ROOT::RVec<double>& adcSampPulseAmp,
                           const ROOT::RVec<double>& adcSampPulseInt,
                           const ROOT::RVec<double>& adcSampPulsePed) -> AdcEventData {

            const float tdcoffset = 0.;            // The tdcoffset will be slot-by-slot
            const float timemean2 = 150.;          // The timemean2 will be a global
            // Converting 4ns bin over 64 samples from FPGA word and dividing to get (ns)
            const float adcSampleDivisions = 16.0; // adcSampleDivisons should be global
            int Npulse = 0;
            AdcEventData eventData;
            eventData.adcResults.resize(NadcCounters);
    
            // Compute the HMS correction time once, from the first ADC channel.
            eventData.HMS_corr_time = adcSampPulseTime[0] -
                                      (adcSampPulseTimeRaw[0] / adcSampleDivisions) -
                                      tdcoffset;
    
            // Optional: Check adcCoutner is a valid slot (0-1079,2000,2001)
            // To implement checkValidSlot

            // Process every ADC entry using the lambda.
            for (int i = 0; i < NadcCounters; ++i) {
                processAdcEntry(eventData.adcResults[i],
                    static_cast<float>(adcSampPulseAmp[i]),
                    static_cast<float>(adcSampPulseTime[i]),
                    static_cast<float>(adcSampPulseInt[i]),
                    static_cast<float>(adcSampPulsePed[i]),
                    timemean2, Npulse
                );
                Npulse++;
            }
            eventData.Npulse = Npulse;
            return eventData;
        },
        {"Ndata.NPS.cal.fly.adcCounter",
         "NPS.cal.fly.adcCounter",
         "NPS.cal.fly.adcSampPulseTime",
         "NPS.cal.fly.adcSampPulseTimeRaw",
         "NPS.cal.fly.adcSampPulseAmp",
         "NPS.cal.fly.adcSampPulseInt",
         "NPS.cal.fly.adcSampPed"}
    );

    auto isGoodPeak = [](double peakTime, double timeref, double timerefacc) -> bool {
        // Check if the absolute difference is within 4.1 ns.
        // TODO - Make 4.1 a global passed in from GlobalManager.
        // Use timeref per slot, load global timerefacc
        return std::fabs(peakTime - timeref - timerefacc) < 4.1;
    };

    auto computeFitParameters = [](const std::vector<PeakContainer>& blockPeaks) -> std::vector<FitResults> {
        std::vector<FitResults> results;
        results.reserve(blockPeaks.size());
        double timeref = 100.0;
        double timerefacc = 50.0;  // constant time offset adjustment

        for (const auto &pc : blockPeaks) {
            FitResults res;
            // Set fallback values in case no pulse qualifies.
            res.good = false;
            res.time = timerefacc;      // fallback time
            res.amplitude = 2.0;         // fallback amplitude
    
            // Iterate over peaks in the container.
            for (int i = 0; i < pc.nPeaks; ++i) {
                // Check "good" criteria: for example, if the absolute difference between the peak time and (timeref + timerefacc) is within 4.1.
                if (std::fabs(pc.peaks[i].time - timeref - timerefacc) < 4.1) {
                    res.good = true;
                    res.time = pc.peaks[i].time;
                    res.amplitude = pc.peaks[i].amplitude;
                    break; // take the first good peak.
                }
            }
            results.push_back(res);
        }
        return results;
    };

    // Step 6: Determine Fits for each peak based on time, amplitude and 'goodness'
    // Perform for each Peak within each DataBlockFloat, which is stored in block_peaks.
    auto dfFitParams = dfAdc.Define("fit_results", computeFitParameters, {"block_peaks"});

    // Step 7: Fit the actual data using Minuit2 and the relevant branches!
    // First I need to check the computeFitParameters and load config correctly.
    // OPTIONAL: Instead of use CPU, stop here and use GPUs for fitting.

    // Step 8: Determine additional quantities (energies, noise, signal widths, etc.)
    // Could also compare fit results from initial fit parameters.

    // Define the final output branches.
    std::vector<std::string> outputBranches = {
        "H.gtr.th",
        "H.gtr.ph",
        "H.gtr.dp",
        "adc_results",
        "block_peaks",
        "fit_results"
    };

    // Should likely flatten objects and store minimal arrays instead of vectors.  
    // Can follow the Ndata pattern like hcana (input).

    std::string outputPath = fileConfig.basePath + fileConfig.outputSubdir + "flattened_output.root";
    std::cout << "Writing flattened output to: " << outputPath << std::endl;

    // Write the final TTree to disk. Note: Snapshot is the terminal action.
    dfFitParams.Snapshot(fileConfig.outputTree, outputPath, outputBranches);
}
