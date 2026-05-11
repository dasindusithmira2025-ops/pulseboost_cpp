#pragma once

#include <QObject>
#include <QSettings>

namespace pulseboost {

class UiPreferences : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool smartAlertsEnabled READ smartAlertsEnabled WRITE setSmartAlertsEnabled NOTIFY smartAlertsEnabledChanged)
    Q_PROPERTY(bool nativeTrayMessages READ nativeTrayMessages WRITE setNativeTrayMessages NOTIFY nativeTrayMessagesChanged)
    Q_PROPERTY(bool minimizeToTrayOnClose READ minimizeToTrayOnClose WRITE setMinimizeToTrayOnClose NOTIFY minimizeToTrayOnCloseChanged)
    Q_PROPERTY(int alertCooldownSeconds READ alertCooldownSeconds WRITE setAlertCooldownSeconds NOTIFY alertCooldownSecondsChanged)
    Q_PROPERTY(bool backgroundMonitoringEnabled READ backgroundMonitoringEnabled WRITE setBackgroundMonitoringEnabled NOTIFY backgroundMonitoringEnabledChanged)
    Q_PROPERTY(bool scheduleEnabled READ scheduleEnabled WRITE setScheduleEnabled NOTIFY scheduleEnabledChanged)
    Q_PROPERTY(QString scheduleMode READ scheduleMode WRITE setScheduleMode NOTIFY scheduleModeChanged)
    Q_PROPERTY(int scheduleHour READ scheduleHour WRITE setScheduleHour NOTIFY scheduleHourChanged)
    Q_PROPERTY(bool scheduleClean READ scheduleClean WRITE setScheduleClean NOTIFY scheduleCleanChanged)
    Q_PROPERTY(bool scheduleRam READ scheduleRam WRITE setScheduleRam NOTIFY scheduleRamChanged)
    Q_PROPERTY(bool scheduleDns READ scheduleDns WRITE setScheduleDns NOTIFY scheduleDnsChanged)
    Q_PROPERTY(bool scheduleSecurity READ scheduleSecurity WRITE setScheduleSecurity NOTIFY scheduleSecurityChanged)
    Q_PROPERTY(QString lastScheduleRunStamp READ lastScheduleRunStamp WRITE setLastScheduleRunStamp NOTIFY lastScheduleRunStampChanged)
    Q_PROPERTY(bool onboardingCompleted READ onboardingCompleted WRITE setOnboardingCompleted NOTIFY onboardingCompletedChanged)

public:
    explicit UiPreferences(QObject *parent = nullptr);

    bool smartAlertsEnabled() const;
    bool nativeTrayMessages() const;
    bool minimizeToTrayOnClose() const;
    int alertCooldownSeconds() const;
    bool backgroundMonitoringEnabled() const;
    bool scheduleEnabled() const;
    QString scheduleMode() const;
    int scheduleHour() const;
    bool scheduleClean() const;
    bool scheduleRam() const;
    bool scheduleDns() const;
    bool scheduleSecurity() const;
    QString lastScheduleRunStamp() const;
    bool onboardingCompleted() const;

    void setSmartAlertsEnabled(bool value);
    void setNativeTrayMessages(bool value);
    void setMinimizeToTrayOnClose(bool value);
    void setAlertCooldownSeconds(int value);
    void setBackgroundMonitoringEnabled(bool value);
    void setScheduleEnabled(bool value);
    void setScheduleMode(const QString &value);
    void setScheduleHour(int value);
    void setScheduleClean(bool value);
    void setScheduleRam(bool value);
    void setScheduleDns(bool value);
    void setScheduleSecurity(bool value);
    void setLastScheduleRunStamp(const QString &value);
    void setOnboardingCompleted(bool value);

signals:
    void smartAlertsEnabledChanged();
    void nativeTrayMessagesChanged();
    void minimizeToTrayOnCloseChanged();
    void alertCooldownSecondsChanged();
    void backgroundMonitoringEnabledChanged();
    void scheduleEnabledChanged();
    void scheduleModeChanged();
    void scheduleHourChanged();
    void scheduleCleanChanged();
    void scheduleRamChanged();
    void scheduleDnsChanged();
    void scheduleSecurityChanged();
    void lastScheduleRunStampChanged();
    void onboardingCompletedChanged();

private:
    template <typename T>
    T readValue(const QString &key, const T &fallback) const {
        return settings_.value(key, QVariant::fromValue(fallback)).template value<T>();
    }

    void writeValue(const QString &key, const QVariant &value);

    mutable QSettings settings_;
};

}  // namespace pulseboost
