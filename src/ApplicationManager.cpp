#include "ApplicationManager.hpp"

ApplicationManager::ApplicationManager(const ApplicationConfig& applicationCfg)
    : config(applicationCfg)
{
}

int ApplicationManager::GetNProcs() const {
    return config.nProcessors;
}

std::string ApplicationManager::GetVersion() const {
    return config.version;
}