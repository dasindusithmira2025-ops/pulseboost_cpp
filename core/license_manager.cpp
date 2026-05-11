#include "PulseBoostAI/core/license_manager.hpp"

#include <Windows.h>
#include <wincrypt.h>

#include <QDate>
#include <QSettings>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <vector>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

namespace {
constexpr int kTrialDays = 14;
const char *kLicenseSettingsGroup = "license";
const char *kTrialStartKey = "trialStartDate";
const char *kTierKey = "tier";
const char *kLicenseKeyKey = "licenseKey";
const char *kCachePath = "logs/license.dat";

std::string trimAscii(const std::string &value) {
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<BYTE> protectData(const std::string &plainText) {
    DATA_BLOB in {};
    in.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(plainText.data()));
    in.cbData = static_cast<DWORD>(plainText.size());

    DATA_BLOB out {};
    if (!CryptProtectData(&in, L"PulseBoostLicense", nullptr, nullptr, nullptr, 0, &out)) {
        return {};
    }

    std::vector<BYTE> encrypted(out.pbData, out.pbData + out.cbData);
    LocalFree(out.pbData);
    return encrypted;
}

std::string unprotectData(const std::vector<BYTE> &cipherText) {
    if (cipherText.empty()) {
        return {};
    }

    DATA_BLOB in {};
    in.pbData = const_cast<BYTE *>(cipherText.data());
    in.cbData = static_cast<DWORD>(cipherText.size());
    DATA_BLOB out {};
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out)) {
        return {};
    }

    std::string plain(reinterpret_cast<char *>(out.pbData), reinterpret_cast<char *>(out.pbData) + out.cbData);
    LocalFree(out.pbData);
    return plain;
}

bool isValidKeyFormat(const std::string &serialKey) {
    static const std::regex pattern("^PULSE-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}$");
    return std::regex_match(serialKey, pattern);
}

QString trialStartDate(QSettings &settings) {
    settings.beginGroup(QString::fromLatin1(kLicenseSettingsGroup));
    QString stored = settings.value(QString::fromLatin1(kTrialStartKey)).toString();
    if (stored.isEmpty()) {
        stored = QDate::currentDate().toString(Qt::ISODate);
        settings.setValue(QString::fromLatin1(kTrialStartKey), stored);
        settings.sync();
    }
    settings.endGroup();
    return stored;
}

}  // namespace

struct LicenseManager::Impl {
    Tier currentTier = Tier::Free;
    std::filesystem::path cachePath = kCachePath;
    mutable QSettings settings {QStringLiteral("PulseBoost"), QStringLiteral("PulseBoostAI")};
};

LicenseManager::LicenseManager() : pImpl(new Impl()) {
    trialStartDate(pImpl->settings);
    currentTier();
}

LicenseManager::~LicenseManager() {
    delete pImpl;
}

LicenseManager::Tier LicenseManager::validateKey(const std::string &serialKey) {
    const std::string trimmed = trimAscii(serialKey);
    if (!isValidKeyFormat(trimmed)) {
        pImpl->currentTier = Tier::Free;
        return pImpl->currentTier;
    }

    if (trimmed.find("LIFE") != std::string::npos || trimmed.find("ENT") != std::string::npos) {
        pImpl->currentTier = Tier::Enterprise;
    } else {
        pImpl->currentTier = Tier::Pro;
    }

    pImpl->settings.beginGroup(QString::fromLatin1(kLicenseSettingsGroup));
    pImpl->settings.setValue(QString::fromLatin1(kTierKey), static_cast<int>(pImpl->currentTier));
    pImpl->settings.setValue(QString::fromLatin1(kLicenseKeyKey), QString::fromStdString(trimmed));
    pImpl->settings.endGroup();
    pImpl->settings.sync();

    std::error_code error;
    std::filesystem::create_directories(pImpl->cachePath.parent_path(), error);
    const std::string payload = std::to_string(static_cast<int>(pImpl->currentTier)) + "|" + trimmed + "|" + currentTimestampUtc();
    const std::vector<BYTE> encrypted = protectData(payload);
    if (!encrypted.empty()) {
        std::ofstream output(pImpl->cachePath, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char *>(encrypted.data()), static_cast<std::streamsize>(encrypted.size()));
    }

    return pImpl->currentTier;
}

LicenseManager::Tier LicenseManager::currentTier() const {
    pImpl->settings.beginGroup(QString::fromLatin1(kLicenseSettingsGroup));
    const int tierCode = pImpl->settings.value(QString::fromLatin1(kTierKey), static_cast<int>(Tier::Free)).toInt();
    pImpl->settings.endGroup();

    if (tierCode >= static_cast<int>(Tier::Free) && tierCode <= static_cast<int>(Tier::Enterprise)) {
        pImpl->currentTier = static_cast<Tier>(tierCode);
    }
    return pImpl->currentTier;
}

int LicenseManager::daysRemainingInTrial() const {
    const QString startDate = trialStartDate(pImpl->settings);
    const QDate start = QDate::fromString(startDate, Qt::ISODate);
    if (!start.isValid()) {
        return kTrialDays;
    }
    const int elapsed = start.daysTo(QDate::currentDate());
    return std::max(0, kTrialDays - elapsed);
}

bool LicenseManager::isTrialExpired() const {
    const Tier tier = currentTier();
    if (tier == Tier::Pro || tier == Tier::Enterprise) {
        return false;
    }
    return daysRemainingInTrial() <= 0;
}

QString LicenseManager::tierLabel() const {
    switch (currentTier()) {
    case Tier::Pro:
        return QStringLiteral("Pro");
    case Tier::Enterprise:
        return QStringLiteral("Lifetime");
    case Tier::Free:
    default:
        return isTrialExpired() ? QStringLiteral("Free") : QStringLiteral("Trial");
    }
}

}  // namespace pulseboost
