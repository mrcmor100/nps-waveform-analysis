#ifndef DATA_ANALYSIS_MANAGER_HPP
#define DATA_ANALYSIS_MANAGER_HPP

#include <TChain.h>
#include "ManagerConfigs.hpp"
#include "FileManager.hpp"
#include "DataTypes.hpp"
#include <ROOT/RDataFrame.hxx>

using RDataFrame_t = ROOT::RDataFrame;

// Forward declarations of your managers.
class GlobalManager;
class ReferenceManager;
class BranchManager;
class FileManager;
class ApplicationManager;

struct ProcessingConfig {
    int nCores = -1;
    double peakTolerance = 0.;
    float timemean = 0.0f;
    float timemean2 = 0.0f;
    float timerefacc = 0.0f;
    std::map<int,double> tdcOffsets;
    std::map<int,double> timeRefs;
};

class DataAnalysisManager {
public:
    // The constructor now takes pointers (or const references) to the other managers.
    DataAnalysisManager(const FileManager* fileMgr,
                        const GlobalManager* globalMgr,
                        const ReferenceManager* refMgr,
                        const BranchManager* branchMgr,
                        const ApplicationManager* applicationMgr);
    void ProcessData();

private:

    void SetupParameters();
    
    TChain* chain;
    int run;
    ProcessingConfig cfg;
    // Pointers to the other managers.
    const GlobalManager* globalManager;
    const ReferenceManager* referenceManager;
    const BranchManager* branchManager;
    const FileManager* fileManager;
    const ApplicationManager* applicationManager;
};

#endif // DATA_ANALYSIS_MANAGER_HPP
