#ifndef APPLICATION_MANAGER_HPP
#define APPLICATION_MANAGER_HPP

#include "ManagerConfigs.hpp"

class ApplicationManager {
public:
    // Accepts a applicationConfig struct at construction.
    ApplicationManager(const ApplicationConfig& applicationCfg);

    // Accessor methods for application parameters.
    int GetNProcs() const;
    std::string GetVersion() const;
    void ApplyConfig(int _run);
    // You can add additional getters as needed.
private:
    int run;
    ApplicationConfig config;
};

#endif // APPLICATION_MANAGER_HPP
