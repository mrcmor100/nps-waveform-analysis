
---

### **3. `CONFIGURATION.md` (Configuration Guide)**
```md
# Configuration Guide

The project uses `config.json` to define:
- Branch names and mapping to user-defined variables
- File paths for waveform reference files
- Run-specific parameters (overrides)

### Example `config.json`
```json
{
  "project_settings": {
    "base_path": "./",
    "waveform_subdir": "waveforms/"
  },
  "branches": [
    { "name": "NPS.cal.fly.adcSampWaveform", "variable": "sampWaveform" }
  ]
}
