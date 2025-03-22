#include "ApplicationManager.hpp"

ApplicationManager::ApplicationManager(const ApplicationConfig& applicationCfg)
    : config(applicationCfg), run(-1)
{
}

int ApplicationManager::GetNProcs() const {
    return config.nProcessors;
}

std::string ApplicationManager::GetVersion() const {
    return config.version;
}

// No run-by-run configuration here
void ApplicationManager::ApplyConfig(int _run) {
    run = _run;
}