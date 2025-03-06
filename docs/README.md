# ROOT Waveform Analysis

This project replaces a ROOT macro with a modular, standalone C++ application using CMake.
It provides a clean separation of concerns by moving configuration into a `config.json`
file and using compiled C++ code instead of an interpreted macro.

## Features
- Modular configuration with JSON (`config.json`)
- Uses CMake for easy compilation and dependency management
- Supports nlohmann JSON, ROOT, and RDataFrame
- Doxygen documentation for API reference

[![Doxygen Documentation](https://img.shields.io/badge/docs-Doxygen-blue)](https://mrcmor100.github.io/nps-waveform-analysis/)
