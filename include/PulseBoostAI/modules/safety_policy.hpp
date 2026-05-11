#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <QJsonObject>

namespace pulseboost {

enum class RiskLevel {
    ReadOnly,
    Low,
    Moderate,
    High,
    Critical,
};

struct SafetyActionDescriptor {
    std::string actionId;
    RiskLevel riskLevel = RiskLevel::ReadOnly;
    bool dryRunSupported = true;
    bool confirmationRequired = false;
    bool backupRequired = false;
    bool rollbackAvailable = false;
    bool auditRequired = true;
    bool advancedOnly = false;
    std::string summary;
};

struct SafetyRequest {
    std::string actionId;
    bool dryRun = true;
    bool confirmed = false;
    bool backupCreated = false;
    bool advancedMode = false;
    std::string actor = "local";
    std::string target;
    QJsonObject requestJson;
};

struct SafetyDecision {
    bool allowed = false;
    std::string reason;
    SafetyActionDescriptor descriptor;
};

struct SafetyAuditEntry {
    std::string actionId;
    std::string status;
    std::string summary;
    RiskLevel riskLevel = RiskLevel::ReadOnly;
    bool dryRun = true;
    QJsonObject requestJson;
    QJsonObject resultJson;
};

class SafetyPolicy {
public:
    explicit SafetyPolicy(std::filesystem::path databasePath = defaultDatabasePath());

    static std::filesystem::path defaultDatabasePath();
    static std::string riskToString(RiskLevel riskLevel);
    static RiskLevel riskFromString(const std::string &riskLevel);

    std::optional<SafetyActionDescriptor> descriptorFor(const std::string &actionId) const;
    SafetyDecision evaluate(const SafetyRequest &request) const;
    bool audit(const SafetyAuditEntry &entry) const;
    QJsonObject descriptorJson(const SafetyActionDescriptor &descriptor) const;
    std::vector<SafetyActionDescriptor> descriptors() const;

private:
    bool ensureDatabase() const;

    std::filesystem::path databasePath_;
};

}  // namespace pulseboost
