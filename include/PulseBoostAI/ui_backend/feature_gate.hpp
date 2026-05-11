#pragma once

#include <QObject>

#include "PulseBoostAI/core/license_manager.hpp"

namespace pulseboost {

class FeatureGate : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isPro READ isPro NOTIFY tierChanged)
    Q_PROPERTY(bool trialExpired READ trialExpired NOTIFY tierChanged)
    Q_PROPERTY(int trialDaysLeft READ trialDaysLeft NOTIFY tierChanged)
    Q_PROPERTY(QString tierLabel READ tierLabel NOTIFY tierChanged)

public:
    explicit FeatureGate(LicenseManager &licenseManager, QObject* parent = nullptr);

    bool isPro() const;
    bool trialExpired() const;
    int trialDaysLeft() const;
    QString tierLabel() const;

    Q_INVOKABLE bool activatePro(const QString &key);
    Q_INVOKABLE void refresh();

signals:
    void tierChanged();

private:
    LicenseManager &licenseManager_;
};

}  // namespace pulseboost
