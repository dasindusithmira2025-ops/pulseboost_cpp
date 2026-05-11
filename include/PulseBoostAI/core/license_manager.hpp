#pragma once

#include <QString>
#include <string>

namespace pulseboost {

class LicenseManager {
public:
    enum class Tier {
        Free,
        Pro,
        Enterprise
    };

    LicenseManager();
    ~LicenseManager();

    Tier validateKey(const std::string &serialKey);
    Tier currentTier() const;
    int daysRemainingInTrial() const;
    bool isTrialExpired() const;
    QString tierLabel() const;

private:
    struct Impl;
    Impl* pImpl;
};

}  // namespace pulseboost
