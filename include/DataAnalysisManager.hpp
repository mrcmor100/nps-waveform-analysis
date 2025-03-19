#ifndef DATA_ANALYSIS_MANAGER_HPP
#define DATA_ANALYSIS_MANAGER_HPP

#include <string>
#include "TChain.h"
#include "ManagerConfigs.hpp"
#include "FileManager.hpp"
#include "DataTypes.hpp"
#include <ROOT/RDataFrame.hxx>

using RDataFrame_t = ROOT::RDataFrame;

class DataAnalysisManager {
public:
    // Constructor accepts the TChain, FileIOConfig (for output paths), and run number.
    DataAnalysisManager(TChain* chain, const FileIOConfig& fileCfg, int run);

    // Main method to process data.
    void ProcessData();

private:
    TChain* chain;
    FileIOConfig fileConfig;
    int run;
};

#endif // DATA_ANALYSIS_MANAGER_HPP
