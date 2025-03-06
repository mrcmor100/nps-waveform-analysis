
---

### **5. `DESIGN.md` (Project Architecture)**
```md
# Project Design & Architecture

## Overview
This project separates concerns into distinct modules:
1. `ConfigManager` - Loads and manages configurations from `config.json`
2. `BranchManager` - Handles branch mapping and linking to ROOT TTree
3. `ReferenceManager` - Loads reference waveforms dynamically

### Transition from CERN ROOT Macro
- Previously, a Clang-compiled macro inside ROOT
- Now a modular, standalone C++ application
- Uses ROOT’s RDataFrame for efficient event processing
