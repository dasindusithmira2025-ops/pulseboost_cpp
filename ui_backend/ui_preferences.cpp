#include "PulseBoostAI/ui_backend/ui_preferences.hpp"

#include <algorithm>

namespace pulseboost {

namespace {
constexpr auto kSmartAlertsKey = "ui/smartAlertsEnabled";
constexpr auto kNativeTrayMessagesKey = "ui/nativeTrayMessages";
constexpr auto kMinimizeToTrayKey = "ui/minimizeToTrayOnClose";
constexpr auto kAlertCooldownKey = "ui/alertCooldownSeconds";
constexpr auto kBackgroundMonitoringKey = "ui/backgroundMonitoringEnabled";
constexpr auto kScheduleEnabledKey = "ui/scheduleEnabled";
constexpr auto kScheduleModeKey = "ui/scheduleMode";
constexpr auto kScheduleHourKey = "ui/scheduleHour";
constexpr auto kScheduleCleanKey = "ui/scheduleClean";
constexpr auto kScheduleRamKey = "ui/scheduleRam";
constexpr auto kScheduleDnsKey = "ui/scheduleDns";
constexpr auto kScheduleSecurityKey = "ui/scheduleSecurity";
constexpr auto kLastScheduleRunKey = "ui/lastScheduleRunStamp";
constexpr auto kOnboardingCompletedKey = "ui/onboardingCompleted";
}

UiPreferences::UiPreferences(QObject *parent)
    : QObject(parent), settings_(QStringLiteral("PulseBoost"), QStringLiteral("PulseBoostAI")) {}

bool UiPreferences::smartAlertsEnabled() const { return readValue<bool>(QString::fromLatin1(kSmartAlertsKey), true); }
bool UiPreferences::nativeTrayMessages() const { return readValue<bool>(QString::fromLatin1(kNativeTrayMessagesKey), true); }
bool UiPreferences::minimizeToTrayOnClose() const { return readValue<bool>(QString::fromLatin1(kMinimizeToTrayKey), false); }
int UiPreferences::alertCooldownSeconds() const { return readValue<int>(QString::fromLatin1(kAlertCooldownKey), 300); }
bool UiPreferences::backgroundMonitoringEnabled() const { return readValue<bool>(QString::fromLatin1(kBackgroundMonitoringKey), true); }
bool UiPreferences::scheduleEnabled() const { return readValue<bool>(QString::fromLatin1(kScheduleEnabledKey), false); }
QString UiPreferences::scheduleMode() const { return readValue<QString>(QString::fromLatin1(kScheduleModeKey), QStringLiteral("daily")); }
int UiPreferences::scheduleHour() const { return readValue<int>(QString::fromLatin1(kScheduleHourKey), 3); }
bool UiPreferences::scheduleClean() const { return readValue<bool>(QString::fromLatin1(kScheduleCleanKey), true); }
bool UiPreferences::scheduleRam() const { return readValue<bool>(QString::fromLatin1(kScheduleRamKey), true); }
bool UiPreferences::scheduleDns() const { return readValue<bool>(QString::fromLatin1(kScheduleDnsKey), false); }
bool UiPreferences::scheduleSecurity() const { return readValue<bool>(QString::fromLatin1(kScheduleSecurityKey), false); }
QString UiPreferences::lastScheduleRunStamp() const { return readValue<QString>(QString::fromLatin1(kLastScheduleRunKey), QString()); }
bool UiPreferences::onboardingCompleted() const { return readValue<bool>(QString::fromLatin1(kOnboardingCompletedKey), false); }

void UiPreferences::writeValue(const QString &key, const QVariant &value) {
    settings_.setValue(key, value);
    settings_.sync();
}

void UiPreferences::setSmartAlertsEnabled(bool value) { if (smartAlertsEnabled() == value) return; writeValue(QString::fromLatin1(kSmartAlertsKey), value); emit smartAlertsEnabledChanged(); }
void UiPreferences::setNativeTrayMessages(bool value) { if (nativeTrayMessages() == value) return; writeValue(QString::fromLatin1(kNativeTrayMessagesKey), value); emit nativeTrayMessagesChanged(); }
void UiPreferences::setMinimizeToTrayOnClose(bool value) { if (minimizeToTrayOnClose() == value) return; writeValue(QString::fromLatin1(kMinimizeToTrayKey), value); emit minimizeToTrayOnCloseChanged(); }
void UiPreferences::setAlertCooldownSeconds(int value) { value = std::clamp(value, 60, 1800); if (alertCooldownSeconds() == value) return; writeValue(QString::fromLatin1(kAlertCooldownKey), value); emit alertCooldownSecondsChanged(); }
void UiPreferences::setBackgroundMonitoringEnabled(bool value) { if (backgroundMonitoringEnabled() == value) return; writeValue(QString::fromLatin1(kBackgroundMonitoringKey), value); emit backgroundMonitoringEnabledChanged(); }
void UiPreferences::setScheduleEnabled(bool value) { if (scheduleEnabled() == value) return; writeValue(QString::fromLatin1(kScheduleEnabledKey), value); emit scheduleEnabledChanged(); }
void UiPreferences::setScheduleMode(const QString &value) { const QString normalized = value.trimmed().toLower(); if (scheduleMode() == normalized) return; writeValue(QString::fromLatin1(kScheduleModeKey), normalized); emit scheduleModeChanged(); }
void UiPreferences::setScheduleHour(int value) { value = std::clamp(value, 0, 23); if (scheduleHour() == value) return; writeValue(QString::fromLatin1(kScheduleHourKey), value); emit scheduleHourChanged(); }
void UiPreferences::setScheduleClean(bool value) { if (scheduleClean() == value) return; writeValue(QString::fromLatin1(kScheduleCleanKey), value); emit scheduleCleanChanged(); }
void UiPreferences::setScheduleRam(bool value) { if (scheduleRam() == value) return; writeValue(QString::fromLatin1(kScheduleRamKey), value); emit scheduleRamChanged(); }
void UiPreferences::setScheduleDns(bool value) { if (scheduleDns() == value) return; writeValue(QString::fromLatin1(kScheduleDnsKey), value); emit scheduleDnsChanged(); }
void UiPreferences::setScheduleSecurity(bool value) { if (scheduleSecurity() == value) return; writeValue(QString::fromLatin1(kScheduleSecurityKey), value); emit scheduleSecurityChanged(); }
void UiPreferences::setLastScheduleRunStamp(const QString &value) { if (lastScheduleRunStamp() == value) return; writeValue(QString::fromLatin1(kLastScheduleRunKey), value); emit lastScheduleRunStampChanged(); }
void UiPreferences::setOnboardingCompleted(bool value) { if (onboardingCompleted() == value) return; writeValue(QString::fromLatin1(kOnboardingCompletedKey), value); emit onboardingCompletedChanged(); }

}  // namespace pulseboost
