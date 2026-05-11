#include "PulseBoostAI/modules/safety_policy.hpp"

#include <cstdlib>
#include <map>

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace pulseboost {

namespace {

using DescriptorMap = std::map<std::string, SafetyActionDescriptor>;

DescriptorMap buildDescriptors() {
    DescriptorMap actions;
    auto add = [&](SafetyActionDescriptor descriptor) {
        actions.emplace(descriptor.actionId, std::move(descriptor));
    };

    add({"junk.clean", RiskLevel::Low, true, false, false, true, true, false, "Quarantine or recycle junk cleanup targets"});
    add({"junk.clean.permanent", RiskLevel::High, true, true, true, false, true, true, "Permanent cleanup deletion"});
    add({"file.delete", RiskLevel::High, true, true, false, false, true, true, "Delete a user-selected file"});
    add({"process.kill", RiskLevel::High, false, true, false, false, true, true, "Terminate a running process"});
    add({"process.suspend", RiskLevel::Moderate, false, true, false, true, true, true, "Suspend a running process"});
    add({"startup.disable", RiskLevel::Moderate, true, true, true, true, true, false, "Disable a startup item"});
    add({"startup.enable", RiskLevel::Low, true, true, false, true, true, false, "Enable a startup item"});
    add({"startup.delay", RiskLevel::Low, true, true, false, true, true, false, "Delay a startup item"});
    add({"network.flush_dns", RiskLevel::Low, true, false, false, true, true, false, "Flush DNS resolver cache"});
    add({"network.optimize_tcp", RiskLevel::High, true, true, true, true, true, true, "Apply global netsh TCP settings"});
    add({"network.winsock_reset", RiskLevel::Critical, true, true, true, false, true, true, "Reset Winsock catalog"});
    add({"network.revert", RiskLevel::High, true, true, true, false, true, true, "Revert PulseBoost network tuning"});
    add({"ram.trim_working_sets", RiskLevel::Moderate, true, true, false, false, true, true, "Trim process working sets manually"});
    add({"ram.flush_standby", RiskLevel::Moderate, true, true, false, false, true, true, "Manual memory pressure relief"});
    add({"ram.saver", RiskLevel::Moderate, true, true, false, false, true, true, "Manual RAM saver mode"});
    add({"disk.optimize", RiskLevel::Moderate, true, true, false, false, true, true, "Trigger Windows disk optimization"});
    add({"restore.create", RiskLevel::Low, false, false, false, true, true, false, "Create a Windows restore point"});
    add({"snapshot.restore", RiskLevel::High, true, true, true, true, true, true, "Restore startup state from a snapshot"});
    add({"tweak.apply", RiskLevel::Moderate, true, true, true, true, true, false, "Apply a registered Windows tweak"});
    add({"tweak.revert", RiskLevel::Moderate, true, true, true, true, true, false, "Revert a registered Windows tweak"});
    add({"game.optimize", RiskLevel::High, true, true, true, true, true, true, "Temporarily tune system state for an active game"});
    add({"game.revert", RiskLevel::Moderate, true, false, false, true, true, false, "Revert temporary game optimization state"});
    add({"schedule.task", RiskLevel::Moderate, true, true, false, true, true, false, "Create or update a scheduled optimization"});
    return actions;
}

const DescriptorMap &descriptorMap() {
    static const DescriptorMap actions = buildDescriptors();
    return actions;
}

QString jsonToCompactString(const QJsonObject &object) {
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

}  // namespace

SafetyPolicy::SafetyPolicy(std::filesystem::path databasePath)
    : databasePath_(std::move(databasePath)) {}

std::filesystem::path SafetyPolicy::defaultDatabasePath() {
    if (const char *appData = std::getenv("APPDATA"); appData != nullptr && *appData != '\0') {
        return std::filesystem::path(appData) / "PulseBoostAI" / "pulseboost.sqlite3";
    }
    return std::filesystem::path("data") / "pulseboost.sqlite3";
}

std::string SafetyPolicy::riskToString(RiskLevel riskLevel) {
    switch (riskLevel) {
        case RiskLevel::ReadOnly: return "read_only";
        case RiskLevel::Low: return "low";
        case RiskLevel::Moderate: return "moderate";
        case RiskLevel::High: return "high";
        case RiskLevel::Critical: return "critical";
    }
    return "unknown";
}

RiskLevel SafetyPolicy::riskFromString(const std::string &riskLevel) {
    if (riskLevel == "low") return RiskLevel::Low;
    if (riskLevel == "moderate") return RiskLevel::Moderate;
    if (riskLevel == "high") return RiskLevel::High;
    if (riskLevel == "critical") return RiskLevel::Critical;
    return RiskLevel::ReadOnly;
}

std::optional<SafetyActionDescriptor> SafetyPolicy::descriptorFor(const std::string &actionId) const {
    const auto found = descriptorMap().find(actionId);
    if (found == descriptorMap().end()) {
        return std::nullopt;
    }
    return found->second;
}

SafetyDecision SafetyPolicy::evaluate(const SafetyRequest &request) const {
    SafetyDecision decision;
    const auto descriptor = descriptorFor(request.actionId);
    if (!descriptor.has_value()) {
        decision.reason = "unknown-action";
        return decision;
    }

    decision.descriptor = *descriptor;
    if (request.dryRun && !descriptor->dryRunSupported) {
        decision.reason = "dry-run-not-supported";
        return decision;
    }
    if (!request.dryRun && descriptor->advancedOnly && !request.advancedMode) {
        decision.reason = "advanced-mode-required";
        return decision;
    }
    if (!request.dryRun && descriptor->confirmationRequired && !request.confirmed) {
        decision.reason = "confirmation-required";
        return decision;
    }
    if (!request.dryRun && descriptor->backupRequired && !request.backupCreated) {
        decision.reason = "backup-required";
        return decision;
    }

    decision.allowed = true;
    decision.reason = "allowed";
    return decision;
}

bool SafetyPolicy::ensureDatabase() const {
    std::error_code error;
    std::filesystem::create_directories(databasePath_.parent_path(), error);
    if (error) {
        return false;
    }

    const QString connectionName = QStringLiteral("pulseboost_safety_%1").arg(reinterpret_cast<quintptr>(this));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    db.setDatabaseName(QString::fromStdString(databasePath_.string()));
    if (!db.open()) {
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }

    QSqlQuery query(db);
    const bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS action_audit_log ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "action_type TEXT NOT NULL,"
        "status TEXT NOT NULL,"
        "summary TEXT NOT NULL,"
        "risk_level TEXT,"
        "dry_run INTEGER NOT NULL DEFAULT 1,"
        "request_json TEXT,"
        "result_json TEXT"
        ")"));
    db.close();
    QSqlDatabase::removeDatabase(connectionName);
    return ok;
}

bool SafetyPolicy::audit(const SafetyAuditEntry &entry) const {
    if (!ensureDatabase()) {
        return false;
    }

    const QString connectionName = QStringLiteral("pulseboost_safety_insert_%1").arg(reinterpret_cast<quintptr>(this));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    db.setDatabaseName(QString::fromStdString(databasePath_.string()));
    if (!db.open()) {
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "INSERT INTO action_audit_log "
        "(created_at, action_type, status, summary, risk_level, dry_run, request_json, result_json) "
        "VALUES (:created_at, :action_type, :status, :summary, :risk_level, :dry_run, :request_json, :result_json)"));
    query.bindValue(QStringLiteral(":created_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":action_type"), QString::fromStdString(entry.actionId));
    query.bindValue(QStringLiteral(":status"), QString::fromStdString(entry.status));
    query.bindValue(QStringLiteral(":summary"), QString::fromStdString(entry.summary));
    query.bindValue(QStringLiteral(":risk_level"), QString::fromStdString(riskToString(entry.riskLevel)));
    query.bindValue(QStringLiteral(":dry_run"), entry.dryRun ? 1 : 0);
    query.bindValue(QStringLiteral(":request_json"), jsonToCompactString(entry.requestJson));
    query.bindValue(QStringLiteral(":result_json"), jsonToCompactString(entry.resultJson));
    const bool ok = query.exec();
    db.close();
    QSqlDatabase::removeDatabase(connectionName);
    return ok;
}

QJsonObject SafetyPolicy::descriptorJson(const SafetyActionDescriptor &descriptor) const {
    return QJsonObject{
        {"actionId", QString::fromStdString(descriptor.actionId)},
        {"riskLevel", QString::fromStdString(riskToString(descriptor.riskLevel))},
        {"dryRunSupported", descriptor.dryRunSupported},
        {"confirmationRequired", descriptor.confirmationRequired},
        {"backupRequired", descriptor.backupRequired},
        {"rollbackAvailable", descriptor.rollbackAvailable},
        {"auditRequired", descriptor.auditRequired},
        {"advancedOnly", descriptor.advancedOnly},
        {"summary", QString::fromStdString(descriptor.summary)},
    };
}

std::vector<SafetyActionDescriptor> SafetyPolicy::descriptors() const {
    std::vector<SafetyActionDescriptor> result;
    result.reserve(descriptorMap().size());
    for (const auto &[_, descriptor] : descriptorMap()) {
        result.push_back(descriptor);
    }
    return result;
}

}  // namespace pulseboost
