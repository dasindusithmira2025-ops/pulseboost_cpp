#pragma once

#include <string>
#include <vector>

namespace pulseboost {

class RegistryOptimizer;
class ServiceManager;

struct TweakDefinition {
    std::string id;
    std::string category;
    std::string name;
    std::string description;
    std::string detailedInfo;
    std::string impact;
    std::string riskLevel;
    bool requiresRestart = false;
    bool isApplied = false;
    bool isApplicable = true;
    std::string notApplicableReason;
};

struct TweakActionResult {
    std::string id;
    bool success = false;
    bool requiresRestart = false;
    bool isApplied = false;
    std::string error;
};

class TweakEngine {
public:
    TweakEngine(RegistryOptimizer &registryOptimizer, ServiceManager &serviceManager);

    std::vector<TweakDefinition> listTweaks() const;
    TweakActionResult applyTweak(const std::string &id) const;
    TweakActionResult revertTweak(const std::string &id) const;
    std::vector<TweakActionResult> applySafeTweaks() const;
    std::vector<TweakActionResult> applyHighImpactTweaks() const;
    std::vector<TweakActionResult> revertAllTweaks() const;
    int appliedCount() const;

private:
    RegistryOptimizer &registryOptimizer_;
    ServiceManager &serviceManager_;
};

}  // namespace pulseboost
