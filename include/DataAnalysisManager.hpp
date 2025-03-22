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

class DataAnalysisManager {
public:
    // The constructor now takes pointers (or const references) to the other managers.
    DataAnalysisManager(int run,
                        TChain* chain,
                        const FileManager* fileMgr,
                        const GlobalManager* globalMgr,
                        const ReferenceManager* refMgr,
                        const BranchManager* branchMgr);
    void ProcessData();

private:
    TChain* chain;
    FileIOConfig fileConfig;
    int run;

    // Pointers to the other managers.
    const GlobalManager* globalManager;
    const ReferenceManager* referenceManager;
    const BranchManager* branchManager;
    const FileManager* fileManager;
};

#endif // DATA_ANALYSIS_MANAGER_HPP
