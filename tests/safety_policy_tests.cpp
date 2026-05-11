#include "PulseBoostAI/modules/safety_policy.hpp"

#include <filesystem>
#include <iostream>

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

int runJunkCleanerTests();

namespace {

int g_failures = 0;

void expect(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++g_failures;
    }
}

std::filesystem::path tempDbPath() {
    auto path = std::filesystem::temp_directory_path() / "pulseboost_safety_policy_test.sqlite3";
    std::error_code error;
    std::filesystem::remove(path, error);
    return path;
}

bool auditRowExists(const std::filesystem::path &dbPath) {
    const QString connectionName = QStringLiteral("audit_test_read");
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    db.setDatabaseName(QString::fromStdString(dbPath.string()));
    if (!db.open()) {
        QSqlDatabase::removeDatabase(connectionName);
        return false;
    }
    QSqlQuery query(db);
    const bool ok = query.exec(QStringLiteral("SELECT COUNT(*) FROM action_audit_log")) && query.next() && query.value(0).toInt() == 1;
    db.close();
    QSqlDatabase::removeDatabase(connectionName);
    return ok;
}

void testPolicyBlocksUnconfirmedAdvancedAction() {
    const auto dbPath = tempDbPath();
    pulseboost::SafetyPolicy policy(dbPath);

    pulseboost::SafetyRequest request;
    request.actionId = "network.optimize_tcp";
    request.dryRun = false;
    request.confirmed = false;
    request.backupCreated = true;
    request.advancedMode = true;

    const auto decision = policy.evaluate(request);
    expect(!decision.allowed, "unconfirmed network tune must be blocked");
    expect(decision.reason == "confirmation-required", "network tune block reason should require confirmation");
}

void testPolicyBlocksNonAdvancedAction() {
    const auto dbPath = tempDbPath();
    pulseboost::SafetyPolicy policy(dbPath);

    pulseboost::SafetyRequest request;
    request.actionId = "ram.trim_working_sets";
    request.dryRun = false;
    request.confirmed = true;
    request.advancedMode = false;

    const auto decision = policy.evaluate(request);
    expect(!decision.allowed, "RAM trim must be advanced/manual only");
    expect(decision.reason == "advanced-mode-required", "RAM trim block reason should require advanced mode");
}

void testPolicyAllowsDryRun() {
    const auto dbPath = tempDbPath();
    pulseboost::SafetyPolicy policy(dbPath);

    pulseboost::SafetyRequest request;
    request.actionId = "junk.clean.permanent";
    request.dryRun = true;

    const auto decision = policy.evaluate(request);
    expect(decision.allowed, "dry-run permanent cleanup plan should be allowed");
}

void testAuditSchema() {
    const auto dbPath = tempDbPath();
    pulseboost::SafetyPolicy policy(dbPath);

    const bool ok = policy.audit(pulseboost::SafetyAuditEntry{
        .actionId = "junk.clean",
        .status = "success",
        .summary = "test audit entry",
        .riskLevel = pulseboost::RiskLevel::Low,
        .dryRun = true,
        .requestJson = QJsonObject{{"test", true}},
        .resultJson = QJsonObject{{"ok", true}},
    });

    expect(ok, "audit insert should succeed");
    expect(auditRowExists(dbPath), "audit table should contain one row");
}

}  // namespace

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    testPolicyBlocksUnconfirmedAdvancedAction();
    testPolicyBlocksNonAdvancedAction();
    testPolicyAllowsDryRun();
    testAuditSchema();
    g_failures += runJunkCleanerTests();
    return g_failures == 0 ? 0 : 1;
}
