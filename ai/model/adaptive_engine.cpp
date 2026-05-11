#include "PulseBoostAI/ai/model/adaptive_engine.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <algorithm>

namespace pulseboost {

namespace {

int intentToInt(Intent intent) {
    return static_cast<int>(intent);
}

template <typename T>
T readSingleValue(const QSqlDatabase &db, const QString &sql, const T &fallback) {
    QSqlQuery query(db);
    if (!query.exec(sql) || !query.next()) {
        return fallback;
    }
    return query.value(0).value<T>();
}

}  // namespace

AdaptiveEngine::AdaptiveEngine(const QString &dbPath) : m_dbPath(dbPath) {
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    m_connectionName = ensureConnectionName();
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open()) {
        m_ready = false;
        return;
    }

    initializeDatabase();
    m_ready = true;
}

AdaptiveEngine::~AdaptiveEngine() {
    const QString connectionName = m_connectionName;
    if (m_db.isOpen()) {
        m_db.close();
    }
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);
}

QString AdaptiveEngine::ensureConnectionName() const {
    return QString("pulsemodel_adaptive_%1").arg(reinterpret_cast<quintptr>(this));
}

bool AdaptiveEngine::isReady() const {
    return m_ready;
}

void AdaptiveEngine::initializeDatabase() {
    QSqlQuery query(m_db);
    query.exec(
        "CREATE TABLE IF NOT EXISTS interactions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "session_id TEXT NOT NULL,"
        "timestamp TEXT NOT NULL,"
        "user_input TEXT NOT NULL,"
        "intent INTEGER NOT NULL,"
        "confidence REAL NOT NULL,"
        "template_used TEXT NOT NULL,"
        "action_offered INTEGER DEFAULT 0,"
        "action_accepted INTEGER DEFAULT -1,"
        "feedback INTEGER DEFAULT 0,"
        "cpu_at_time REAL,"
        "ram_at_time REAL,"
        "health_at_time REAL,"
        "follow_up TEXT"
        ");");

    query.exec(
        "CREATE TABLE IF NOT EXISTS template_weights ("
        "template_id TEXT PRIMARY KEY,"
        "weight REAL DEFAULT 1.0,"
        "shown_count INTEGER DEFAULT 0,"
        "positive_count INTEGER DEFAULT 0,"
        "negative_count INTEGER DEFAULT 0,"
        "action_accepted INTEGER DEFAULT 0,"
        "action_declined INTEGER DEFAULT 0,"
        "last_updated TEXT"
        ");");

    query.exec(
        "CREATE TABLE IF NOT EXISTS user_patterns ("
        "key TEXT PRIMARY KEY,"
        "value TEXT NOT NULL,"
        "updated_at TEXT NOT NULL"
        ");");
}

void AdaptiveEngine::recordInteraction(const InteractionRecord &record) {
    if (!m_ready) {
        return;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO interactions(session_id,timestamp,user_input,intent,confidence,template_used,action_offered,"
        "action_accepted,feedback,cpu_at_time,ram_at_time,health_at_time,follow_up)"
        " VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)");
    query.addBindValue(record.sessionId);
    query.addBindValue(record.timestamp.isValid() ? record.timestamp.toString(Qt::ISODate) : QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(record.userInput);
    query.addBindValue(intentToInt(record.detectedIntent));
    query.addBindValue(record.intentConfidence);
    query.addBindValue(record.templateUsed);
    query.addBindValue(record.actionOffered ? 1 : 0);
    query.addBindValue(record.actionOffered ? (record.actionAccepted ? 1 : 0) : -1);
    query.addBindValue(record.feedbackScore);
    query.addBindValue(record.cpuAtTime);
    query.addBindValue(record.ramAtTime);
    query.addBindValue(record.healthAtTime);
    query.addBindValue(record.followUpQuestion);
    query.exec();

    if (!record.templateUsed.isEmpty()) {
        incrementCounter(record.templateUsed, "shown_count");
        if (record.feedbackScore > 0) {
            incrementCounter(record.templateUsed, "positive_count");
            updateTemplateWeight(record.templateUsed, 0.10F * (2.0F - getTemplateWeight(record.templateUsed)));
        } else if (record.feedbackScore < 0) {
            incrementCounter(record.templateUsed, "negative_count");
            updateTemplateWeight(record.templateUsed, -0.15F * getTemplateWeight(record.templateUsed));
        } else {
            const float current = getTemplateWeight(record.templateUsed);
            const float target = 1.0F;
            updateTemplateWeight(record.templateUsed, (target - current) * (1.0F - DECAY_RATE));
        }
    }
}

void AdaptiveEngine::recordThumbsUp(const QString &sessionId, const QString &templateId) {
    Q_UNUSED(sessionId)
    if (!m_ready || templateId.isEmpty()) {
        return;
    }
    incrementCounter(templateId, "positive_count");
    updateTemplateWeight(templateId, 0.10F * (2.0F - getTemplateWeight(templateId)));
}

void AdaptiveEngine::recordThumbsDown(const QString &sessionId, const QString &templateId) {
    Q_UNUSED(sessionId)
    if (!m_ready || templateId.isEmpty()) {
        return;
    }
    incrementCounter(templateId, "negative_count");
    updateTemplateWeight(templateId, -0.15F * getTemplateWeight(templateId));
}

void AdaptiveEngine::recordActionAccepted(const QString &sessionId, const QString &templateId) {
    Q_UNUSED(sessionId)
    if (!m_ready || templateId.isEmpty()) {
        return;
    }
    incrementCounter(templateId, "action_accepted");
    updateTemplateWeight(templateId, 0.05F);
}

void AdaptiveEngine::recordActionDeclined(const QString &sessionId, const QString &templateId) {
    Q_UNUSED(sessionId)
    if (!m_ready || templateId.isEmpty()) {
        return;
    }
    incrementCounter(templateId, "action_declined");
    updateTemplateWeight(templateId, -0.05F);
}

void AdaptiveEngine::recordFollowUp(const QString &sessionId, const QString &followUpText) {
    if (!m_ready || sessionId.isEmpty()) {
        return;
    }

    QSqlQuery query(m_db);
    query.prepare("UPDATE interactions SET follow_up=? WHERE session_id=? ORDER BY id DESC LIMIT 1");
    query.addBindValue(followUpText);
    query.addBindValue(sessionId);
    query.exec();
}

void AdaptiveEngine::incrementCounter(const QString &templateId, const QString &column) {
    if (!m_ready || templateId.isEmpty()) {
        return;
    }

    QSqlQuery seed(m_db);
    seed.prepare("INSERT OR IGNORE INTO template_weights(template_id,weight,last_updated) VALUES(?,?,?)");
    seed.addBindValue(templateId);
    seed.addBindValue(1.0);
    seed.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    seed.exec();

    QSqlQuery query(m_db);
    query.prepare(QString("UPDATE template_weights SET %1 = %1 + 1, last_updated=? WHERE template_id=?").arg(column));
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(templateId);
    query.exec();
}

void AdaptiveEngine::updateTemplateWeight(const QString &templateId, float delta) {
    if (!m_ready || templateId.isEmpty()) {
        return;
    }

    const float current = getTemplateWeight(templateId);
    const float updated = std::clamp(current + delta, WEIGHT_FLOOR, WEIGHT_CEILING);

    QSqlQuery seed(m_db);
    seed.prepare("INSERT OR IGNORE INTO template_weights(template_id,weight,last_updated) VALUES(?,?,?)");
    seed.addBindValue(templateId);
    seed.addBindValue(current);
    seed.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    seed.exec();

    QSqlQuery query(m_db);
    query.prepare("UPDATE template_weights SET weight=?, last_updated=? WHERE template_id=?");
    query.addBindValue(updated);
    query.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.addBindValue(templateId);
    query.exec();
}

float AdaptiveEngine::getTemplateWeight(const QString &templateId) const {
    if (!m_ready || templateId.isEmpty()) {
        return 1.0F;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT weight FROM template_weights WHERE template_id=?");
    query.addBindValue(templateId);
    if (!query.exec() || !query.next()) {
        return 1.0F;
    }
    return query.value(0).toFloat();
}

QHash<QString, float> AdaptiveEngine::getAllWeights() const {
    QHash<QString, float> weights;
    if (!m_ready) {
        return weights;
    }

    QSqlQuery query(m_db);
    if (!query.exec("SELECT template_id, weight FROM template_weights")) {
        return weights;
    }
    while (query.next()) {
        weights.insert(query.value(0).toString(), query.value(1).toFloat());
    }
    return weights;
}

QString AdaptiveEngine::getMostEffectiveTemplateForIntent(Intent intent) const {
    if (!m_ready) {
        return {};
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT template_used, SUM(CASE WHEN feedback=1 THEN 1 ELSE 0 END) AS positives "
        "FROM interactions WHERE intent=? GROUP BY template_used ORDER BY positives DESC LIMIT 1");
    query.addBindValue(intentToInt(intent));
    if (!query.exec() || !query.next()) {
        return {};
    }
    return query.value(0).toString();
}

bool AdaptiveEngine::userPrefersVerboseResponses() const {
    if (!m_ready) {
        return false;
    }
    const int followUps = readSingleValue<int>(m_db, "SELECT COUNT(1) FROM interactions WHERE follow_up IS NOT NULL AND follow_up <> ''", 0);
    const int total = std::max(1, getTotalInteractions());
    return static_cast<float>(followUps) / static_cast<float>(total) > 0.25F;
}

bool AdaptiveEngine::userPrefersActionSuggestions() const {
    if (!m_ready) {
        return true;
    }
    const int offered = readSingleValue<int>(m_db, "SELECT COUNT(1) FROM interactions WHERE action_offered=1", 0);
    if (offered == 0) {
        return true;
    }
    const int accepted = readSingleValue<int>(m_db, "SELECT COUNT(1) FROM interactions WHERE action_offered=1 AND action_accepted=1", 0);
    return static_cast<float>(accepted) / static_cast<float>(offered) >= 0.35F;
}

QString AdaptiveEngine::getUserPrimaryIssuePattern() const {
    if (!m_ready) {
        return "unknown";
    }

    QSqlQuery query(m_db);
    if (!query.exec(
            "SELECT CASE "
            "WHEN ram_at_time >= cpu_at_time AND ram_at_time >= health_at_time THEN 'memory' "
            "WHEN cpu_at_time >= ram_at_time AND cpu_at_time >= health_at_time THEN 'cpu' "
            "ELSE 'mixed' END AS issue, COUNT(1) AS c "
            "FROM interactions GROUP BY issue ORDER BY c DESC LIMIT 1")) {
        return "unknown";
    }
    if (!query.next()) {
        return "unknown";
    }
    return query.value(0).toString();
}

int AdaptiveEngine::getTotalInteractions() const {
    if (!m_ready) {
        return 0;
    }
    return readSingleValue<int>(m_db, "SELECT COUNT(1) FROM interactions", 0);
}

float AdaptiveEngine::getOverallSatisfactionRate() const {
    if (!m_ready) {
        return 0.0F;
    }
    const int positives = readSingleValue<int>(m_db, "SELECT COUNT(1) FROM interactions WHERE feedback=1", 0);
    const int rated = readSingleValue<int>(m_db, "SELECT COUNT(1) FROM interactions WHERE feedback<>0", 0);
    if (rated == 0) {
        return 0.0F;
    }
    return static_cast<float>(positives) / static_cast<float>(rated);
}

QVector<QString> AdaptiveEngine::getFrequentlyAskedTopics() const {
    QVector<QString> topics;
    if (!m_ready) {
        return topics;
    }

    QSqlQuery query(m_db);
    if (!query.exec("SELECT intent, COUNT(1) AS c FROM interactions GROUP BY intent ORDER BY c DESC LIMIT 5")) {
        return topics;
    }
    while (query.next()) {
        topics.push_back(QString("intent_%1").arg(query.value(0).toInt()));
    }
    return topics;
}

bool AdaptiveEngine::resetLearnedData() {
    if (!m_ready) {
        return false;
    }
    QSqlQuery query(m_db);
    const bool ok1 = query.exec("DELETE FROM interactions");
    const bool ok2 = query.exec("DELETE FROM template_weights");
    const bool ok3 = query.exec("DELETE FROM user_patterns");
    return ok1 && ok2 && ok3;
}

}  // namespace pulseboost
