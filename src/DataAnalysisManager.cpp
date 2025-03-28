#include "DataAnalysisManager.hpp"
#include "WaveformFitter.hpp"
#include "GlobalManager.hpp"
#include "ApplicationManager.hpp"
#include "ReferenceManager.hpp"
#include "BranchManager.hpp"
#include "FileManager.hpp"

#include "Math/Interpolator.h"
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RVec.hxx>
#include <iostream>
#include <algorithm>  // for std::min

// Constructor: store pointers to the other managers so we can use them in processing.
DataAnalysisManager::DataAnalysisManager(const FileManager* fileMgr,
                                         const GlobalManager* globalMgr,
                                         const ReferenceManager* refMgr,
                                         const BranchManager* branchMgr,
                                         const ApplicationManager* appMgr)
    : run(-1), chain(nullptr), fileManager(fileMgr), globalManager(globalMgr), 
      referenceManager(refMgr), branchManager(branchMgr),
      applicationManager(appMgr)
{
}

void DataAnalysisManager::SetupParameters() {
    chain = fileManager->GetInputChain();
    if(applicationManager) {
        cfg.nCores = applicationManager->GetNProcs();
    }
    if (globalManager) {
        cfg.peakTolerance = globalManager->GetPeakTolerance();
        cfg.timerefacc = globalManager->GetTimeRefAcc();
    }
    if (referenceManager) {
        cfg.timeRefs = referenceManager->GetTimeRefs();
        cfg.tdcOffsets = referenceManager->GettdcOffsets();
        cfg.timemean = referenceManager->GetTimeMean();
        cfg.timemean2 = referenceManager->GetTimeMean2();
    }
}

void DataAnalysisManager::ProcessData() {
    this->SetupParameters();
    // Check WaveformAnalysis Version

    // Enable multi-threading using the number of cores from configuration.
    ROOT::EnableImplicitMT(cfg.nCores);

    // Create the initial RDataFrame from the TChain.
    ROOT::RDataFrame df(*chain);

    // Step 1: Filter events based on tracking cuts.
    //TODO: Specify cut values in configuration
    auto dfFiltered = df.Filter(
        [](double th, double ph, double dp) {
            return std::fabs(th) < 0.08 && std::fabs(ph) < 0.04 && std::fabs(dp) < 10;
        },
        {"H.gtr.th", "H.gtr.ph", "H.gtr.dp"}
    );

    // Step 2: Convert the raw waveform data from double to float.
    // We define a new branch "float_waveform_data" that converts the raw data.
    auto dfWaveform = dfFiltered.Define("float_waveform_data",
        [](const ROOT::RVec<double>& waveform, int nsamp) -> std::vector<float> {
            std::vector<float> float_data;
            float_data.reserve(nsamp);
            std::transform(waveform.begin(), waveform.end(), std::back_inserter(float_data),
                           [](double x) { return static_cast<float>(x); });
            return float_data;
        },
        {"NPS.cal.fly.adcSampWaveform", "Ndata.NPS.cal.fly.adcSampWaveform"}
    );

    // Step 3a: Determine Number of Waveforms assuming DataBlockSized DataBlocks
    // Save the size for use in future steps.
    auto dfSizeBlocks = dfWaveform.Define("n_channels",
        [_DataBlockSize = DataBlockSize] (int adcSampWaveformSize) {
            return adcSampWaveformSize / static_cast<int>(_DataBlockSize);
        },
        {"Ndata.NPS.cal.fly.adcSampWaveform"}
    );

    // Step 3b: Make DataBlockFloat structures from raw float arrays.
    // Future steps can operate directly on DataBlockFloats within each event.
    auto dfFlattened = dfSizeBlocks.Define("float_blocks",
        [_DataBlockSize = DataBlockSize](const std::vector<float>& float_data, int nBlocks) -> std::vector<DataBlockFloat> {
            std::vector<DataBlockFloat> blocks;
            blocks.reserve(nBlocks);
            for (int i = 0; i < nBlocks; ++i) {
                size_t offset = i * _DataBlockSize;
                float channel = float_data[offset];
                // Only Save Waveforms from the block channels.
                // Should add a more robust method here like isBlock(channel).
                if(channel == 2000 || channel == 2001) {continue;}
                float n_samples = float_data[offset + 1];
                size_t num_to_copy = std::min<size_t>(n_samples, NumSamples);
                const float* waveform_ptr = &float_data[offset + 2];
                blocks.emplace_back(channel, n_samples, waveform_ptr, num_to_copy,true);
            }
            return blocks;
        },
        {"float_waveform_data", "n_channels"}
    );

    // Lambda for checking peak quality using the tolerance from GlobalManager.
    auto isGoodPeak = [_timeref = cfg.timeRefs, 
                       _peakTolerance = cfg.peakTolerance,
                       _timerefacc = cfg.timerefacc]
        (int block_number, float peakTime) -> bool {
        //std::cout << peakTime << " " << _timeref.at(block_number) << " " <<_timerefacc << " " << _peakTolerance << '\n';
        return std::fabs(peakTime - _timeref.at(block_number) - _timerefacc) < _peakTolerance;
    };

    auto findPeaks = [_isGoodPeak = isGoodPeak,
                      _NumSamples = NumSamples,
                      _timeref = cfg.timeRefs,
                      _peakTolerance = cfg.peakTolerance,
                      _timerefacc = cfg.timerefacc]
        (const DataBlockFloat &block) -> PeakContainer {
        PeakContainer result;
        result.block = block.block_id;
        for (int it = 0; it < _NumSamples - 6; ++it) {
            if (block.data[it] < block.data[it+1] &&
                block.data[it+1] < block.data[it+2] &&
                block.data[it+2] <= block.data[it+3] &&
                block.data[it+3] >= block.data[it+4] &&
                block.data[it+4] > 0) {
                if (result.nPeaks < PeakContainer::maxPeaks) {
                    // Load PeakContainer::maxPeaks from config
                    result.peaks[result.nPeaks].amplitude = block.data[it+3];
                    result.peaks[result.nPeaks].time = static_cast<float>(it+3);
                    result.peaks[result.nPeaks].good = _isGoodPeak(block.block_id, 
                        4.*static_cast<float>(it+3));
                    result.nPeaks++;
                } else {
                    result.peakOverflow = 1;
                    break;
                }
                it += 4;
            }
        }
        return result;
    };

    // Step 4: Find Peaks within each DataBlockFloat.Data()
    // Find all the peak amplitude and time within the individual DataBlockFloat objects.
    auto dfPeaks = dfFlattened.Define("block_peaks",
        [_findPeaks = findPeaks](const std::vector<DataBlockFloat>& blocks) -> std::vector<PeakContainer> {
            std::vector<PeakContainer> peaks;
            peaks.reserve(blocks.size());
            for (const auto &block : blocks) {
                peaks.push_back(_findPeaks(block));
            }
            return peaks;
        },
        {"float_blocks"}
    );

    // Lambda to process a single ADC entry.
    auto processAdcEntry = [_timemean2 = cfg.timemean2](AdcResult& res,
        float pulseAmp, float pulseTime, float pulseInt,
        float pulsePed, int Npulse) {
        // If it's the first pulse or the new pulse is closer to the reference time, update the result.
        if (Npulse == 1 || std::fabs(res.Samptime - _timemean2) > 
                           std::fabs(pulseTime    - _timemean2)) {
            res.Sampampl = pulseAmp;
            res.Samptime = pulseTime;
            res.Sampener = pulseInt;
            res.Sampped  = pulsePed;
        }
    };

    // Step 5: Calculate ADC parameters.
    // Here we encapsulate the ADC logic (like computing HMS time corrections,
    auto dfAdc = dfPeaks.Define("adc_results_blocks",
        [_processAdcEntry = processAdcEntry,
         _tdcoffset = cfg.tdcOffsets,
         _adcSampleDivisions = adcSampleDivisions,
         timemean2 = cfg.timemean2](int NadcCounters,
                           const ROOT::RVec<double>& adcCounters,
                           const ROOT::RVec<double>& adcSampPulseTime,
                           const ROOT::RVec<double>& adcSampPulseTimeRaw,
                           const ROOT::RVec<double>& adcSampPulseAmp,
                           const ROOT::RVec<double>& adcSampPulseInt,
                           const ROOT::RVec<double>& adcSampPulsePed) -> AdcEventData {
            // Pull values from GlobalManager (or set defaults)
            int Npulse = 0;
            AdcEventData eventData;
            eventData.adcResults.resize(NadcCounters);
            
            eventData.HMS_corr_time = adcSampPulseTime[0] -
                                      (adcSampPulseTimeRaw[0] / _adcSampleDivisions) -
                                      _tdcoffset.at(adcCounters[0]);
            
                                      // The _tdcoffset for adcCounters[0],
                                      // IDK what that is.
                                      // Maybe the block that fired off the event?
    
            for (int i = 0; i < NadcCounters; ++i) {
                _processAdcEntry(eventData.adcResults[i],
                                static_cast<float>(adcSampPulseAmp[i]),
                                static_cast<float>(adcSampPulseTime[i]),
                                static_cast<float>(adcSampPulseInt[i]),
                                static_cast<float>(adcSampPulsePed[i]),
                                Npulse);
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

    auto computeFitParameters = [_timeref = cfg.timeRefs,
                                _timerefacc = cfg.timerefacc]
            (const PeakContainer& pc) -> BlockFitParameters {
        
        BlockFitParameters params;
        
        // Initialize the pedestal parameter (similar to setting parameter 1 in the old code)
        params.block_pedestal.pedestal = 0.;
        params.block_pedestal.ped_lower_limit = -100.;
        params.block_pedestal.ped_upper_limit = 100.;
        params.nPeaks = pc.nPeaks;
        params.block = pc.block;

        // Set a default state for the block
        params.good = false;
        // Optionally, initialize all peak parameters with safe defaults.
        if(pc.nPeaks == 0) {
            params.peak_parameters[0].time = _timerefacc;
            params.peak_parameters[0].time_lower_limit = _timerefacc - 4;
            params.peak_parameters[0].time_upper_limit = _timerefacc + 4;
            params.peak_parameters[0].amplitude = 2.0;
            params.peak_parameters[0].amplitude_lower_limit = 0.05;
            params.peak_parameters[0].amplitude_upper_limit = 10;
        }
        
        // Process peaks in the block and fill the first good peak parameter.
        // (Adapt this logic if you want to process more than one peak per block.)
        for (int i = 0; i < pc.nPeaks; ++i) {
            if (pc.peaks[i].good) {
                params.good = true;
                // Compute the corrected time relative to the time reference.
                double correctedTime = pc.peaks[i].time - _timeref.at(pc.block);
                params.peak_parameters[i].time = correctedTime;
                params.peak_parameters[i].time_lower_limit = correctedTime - 3;
                params.peak_parameters[i].time_upper_limit = correctedTime + 3;
                
                double amplitude = pc.peaks[i].amplitude;
                params.peak_parameters[i].amplitude = amplitude;
                params.peak_parameters[i].amplitude_lower_limit = amplitude * 0.2;
                params.peak_parameters[i].amplitude_upper_limit = amplitude * 3;
            }
        }
        if(params.good == false) {
            int lp  = pc.nPeaks -1; // TODO: Fix this to be nPeaks, but causes...
            // Stack Smashing currently if nPeaks is PeakContainer::maxPeaks.
            // Still want to try and fit a pulse in time.
            // Replace the last peaks attempted fit?
            params.peak_parameters[lp].time = _timerefacc;
            params.peak_parameters[lp].time_lower_limit = _timerefacc - 4;
            params.peak_parameters[lp].time_upper_limit = _timerefacc + 4;
            params.peak_parameters[lp].amplitude = 2.0;
            params.peak_parameters[lp].amplitude_lower_limit = 0.05;
            params.peak_parameters[lp].amplitude_upper_limit = 10;
        }
        return params;
    };

    // Step 6: Determine Fits for each peak based on time, amplitude and 'goodness'
    // Perform for each Peak within each DataBlockFloat, which is stored in block_peaks.
    auto dfFitParams = dfAdc.Define("fit_params_blocks", 
        [_computeFitParameters = computeFitParameters] (const std::vector<PeakContainer>& blockPeaks) -> std::vector<BlockFitParameters> {
            std::vector<BlockFitParameters> results;
            results.reserve(blockPeaks.size());
            for (const auto &pc : blockPeaks) {
                results.push_back(_computeFitParameters(pc));
            }
            return results;
        },
        {"block_peaks"}
    );

    // Step 7a: Create a histogram from a block to use the TH1::Fit method
    // OPTIONAL: Instead of use CPU, stop here and use GPUs for fitting.
    auto makeHistogramFromBlock = [](const DataBlockFloat& block, const std::string& name) 
        -> std::shared_ptr<TH1F> {
        auto hist = std::make_shared<TH1F>(
            name.c_str(), "Waveform Histogram",
            NumSamples, -0.5, NumSamples + 0.5
        );
    
        for (int i = 0; i < NumSamples; ++i) {
            hist->SetBinContent(i + 1, block.data[i]);
            hist->SetBinError(i + 1, block.errors[i]);
        }
    
        return hist;
    };

    auto dfHistograms = dfFitParams.Define("histograms_blocks",
        [makeHistogramFromBlock](const std::vector<DataBlockFloat>& float_blocks) -> std::vector<std::shared_ptr<TH1F>> {
            std::vector<std::shared_ptr<TH1F>> results;
            results.reserve(float_blocks.size());
            std::string name;
            for (size_t i = 0; i < float_blocks.size(); ++i) {
                name = "hist_block" + std::to_string(static_cast<int>(float_blocks[i].block_id));
                results.push_back(makeHistogramFromBlock(float_blocks[i], name));
            }

            return results;
        },
        {"float_blocks"}
    );

    // Step 7b: Setup a fit function with the fit_params_blocks.
    // Make sure to save real time required to fit, and store relevant metadata.
    //  Extract chisq, peaks, etc.
    auto makeTF1FromFitParams = [this](const BlockFitParameters& fitParams)
    -> TF1*
    {
        int blockId = fitParams.block;
        const ROOT::Math::Interpolator* interp = referenceManager->GetInterpolator(blockId);
        WaveformFitter fitter = WaveformFitter(interp, 1., 109.);
        return fitter.CreateTF1ForBlock(fitParams);
    };


    auto dfTF1Blocks = dfHistograms.Define("tf1_blocks",
        [makeTF1FromFitParams](const std::vector<BlockFitParameters>& fitParamsVec)
            -> std::vector<TF1*>
        {
            std::vector<TF1*> tf1Vec;
            tf1Vec.reserve(fitParamsVec.size());
    
            for (size_t i = 0; i < fitParamsVec.size(); ++i)
            {
                tf1Vec.emplace_back(makeTF1FromFitParams(fitParamsVec[i]));
            }
    
            return tf1Vec;
        },
        {"fit_params_blocks"}
    );

    auto dfFitBlocks = dfTF1Blocks.Define("the_fit_applied",
        [](const std::vector<TF1*>& tf1Vec,
           const std::vector<std::shared_ptr<TH1F>> histVec)
            -> TF1*
        {
            if (tf1Vec.empty() || histVec.empty() || !tf1Vec[0] || !histVec[0]) {
                return nullptr;
            }
    
            auto* tf1 = tf1Vec[0];
            auto* hist = histVec[0].get();
    
            hist->Fit(tf1, "Q"); // "Q" for quiet. Add "R" for range only if needed
    
            return tf1;
        },
        {"tf1_blocks","histograms_blocks"}
    );

    // Step 8: Determine additional quantities (energies, noise, signal widths, etc.)
    // Could also compare fit results from initial fit parameters.

    // Define the final output branches.
    std::vector<std::string> outputBranches = {
        "H.gtr.th",
        "H.gtr.ph",
        "H.gtr.dp",
        "adc_results_blocks",
        "block_peaks",
        "fit_params_blocks",
        "float_blocks"
    };

    // Should likely flatten objects and store minimal arrays instead of vectors.  
    // Can follow the Ndata pattern like hcana (input).

    // Write the final TTree to disk. Note: Snapshot is the terminal action.
    //if(fileManager)
    //ROOT::RDF::SaveGraph(dfHistograms);
    dfFitBlocks.Snapshot("TOUT", "output/tmp.root", outputBranches);
}
