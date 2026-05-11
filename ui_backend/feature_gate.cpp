#include "PulseBoostAI/ui_backend/feature_gate.hpp"

namespace pulseboost {

FeatureGate::FeatureGate(LicenseManager &licenseManager, QObject* parent)
    : QObject(parent), licenseManager_(licenseManager) {}

bool FeatureGate::isPro() const {
    const auto tier = licenseManager_.currentTier();
    return tier == LicenseManager::Tier::Pro || tier == LicenseManager::Tier::Enterprise;
}

bool FeatureGate::trialExpired() const {
    return licenseManager_.isTrialExpired();
}

int FeatureGate::trialDaysLeft() const {
    return licenseManager_.daysRemainingInTrial();
}

QString FeatureGate::tierLabel() const {
    return licenseManager_.tierLabel();
}

bool FeatureGate::activatePro(const QString &key) {
    auto tier = licenseManager_.validateKey(key.toStdString());
    emit tierChanged();
    return tier == LicenseManager::Tier::Pro || tier == LicenseManager::Tier::Enterprise;
}

void FeatureGate::refresh() {
    emit tierChanged();
}

}  // namespace pulseboost
