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
    // Create an RDataFrame from the chain. Assume the tree name is provided in fileConfig.inputTree.
    ROOT::RDataFrame df(*chain);

    // Filter the events using inline lambda. 
    // Note: We assume branches "H.gtr.th", "H.gtr.ph", and "H.gtr.dp" are available.
    auto dfFiltered = df.Filter(
        [](double th, double ph, double dp) {
            return TMath::Abs(th) < 0.08 && TMath::Abs(ph) < 0.04 && TMath::Abs(dp) < 10;
        },
        {"H.gtr.th", "H.gtr.ph", "H.gtr.dp"}
    );

    // Now, process the "SampWaveForm" branch.
    // For demonstration, assume "SampWaveForm" is stored as a std::vector<double>.
    // We will flatten this by, for example, taking the first element from each block.
    // (In a full implementation, you might loop in blocks or build a structure.)
    auto dfProcessed = dfFiltered.Define("waveform_block_data",
        [](const ROOT::RVec<double>& waveform, int nsamp) -> std::vector<double> {
            std::vector<double> flat_data;
            constexpr int BLOCK_SIZE = 112;
    
            for (int i = 0; i < nsamp; i += BLOCK_SIZE) {
                int end = std::min(i + BLOCK_SIZE, nsamp);
                for (int j = i; j < end; j++) {
                    flat_data.push_back(waveform[j]); // Flatten block data
                }
            }
    
            return flat_data;
        },
        {"NPS.cal.fly.adcSampWaveform", "Ndata.NPS.cal.fly.adcSampWaveform"}
    );
    
    

    // Specify the branches that should be written to the output file.
    std::vector<std::string> outputBranches = {
        "H.gtr.th",
        "H.gtr.ph",
        "H.gtr.dp"
    };

    // Determine the output file path.
    std::string outputPath = fileConfig.basePath + fileConfig.outputSubdir + "flattened_output.root";
    std::cout << "Writing flattened output to: " << outputPath << std::endl;

    // Write the flattened tree to an output file.
    // "FlattenedTree" is the name of the tree in the output file.
    dfProcessed.Snapshot(fileConfig.outputTree, outputPath, outputBranches);
}
