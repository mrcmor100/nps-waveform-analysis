#ifdef __CINT__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// Generate dictionaries for your template instantiations:
#pragma link C++ class DataBlock<float, NumSamples>+;
#pragma link C++ class DataBlock<double, NumSamples>+;
#pragma link C++ class std::vector<DataBlock<float, NumSamples> >+;
#pragma link C++ class std::vector<AdcResult>+;
#pragma link C++ class Peak+;
#pragma link C++ class PeakContainer+;
#pragma link C++ class std::vector<PeakContainer>+;
#pragma link C++ class std::vector<FitResults>+;
#pragma link C++ class std::vector<AdcResult>+;
#pragma link C++ class AdcEventData+;
#pragma link C++ class BlockFitParameters+;
#pragma link C++ class PeakFitParameter+;
#pragma link C++ class PedestalFitParameter+;
#pragma link C++ class std::vector<BlockFitParameters>+;
#endif
