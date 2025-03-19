#ifdef __CINT__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

// Generate dictionaries for your template instantiations:
#pragma link C++ class DataBlock<float, NumSamples>+;
#pragma link C++ class DataBlock<double, NumSamples>+;
#pragma link C++ class std::vector<DataBlock<float, NumSamples> >+;
#endif
