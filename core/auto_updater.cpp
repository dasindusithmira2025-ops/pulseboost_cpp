#include "PulseBoostAI/core/auto_updater.hpp"

#include <Windows.h>
#include <WinInet.h>
#include <shellapi.h>
#include <urlmon.h>
#include <wincrypt.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <vector>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStringList>

#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "Advapi32.lib")

namespace pulseboost {

namespace {

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool parseManifest(const std::string &manifest,
                   std::string *version,
                   std::string *url,
                   std::string *sha256,
                   std::string *error) {
    QJsonParseError parseError {};
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(manifest), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error != nullptr) {
            *error = (QStringLiteral("malformed manifest JSON: %1").arg(parseError.errorString())).toStdString();
        }
        return false;
    }

    if (!document.isObject()) {
        if (error != nullptr) {
            *error = "manifest root is not an object";
        }
        return false;
    }

    const QJsonObject root = document.object();
    static const QStringList requiredFields {
        QStringLiteral("version"),
        QStringLiteral("url"),
        QStringLiteral("releaseNotes"),
        QStringLiteral("minOsVersion"),
        QStringLiteral("sha256")
    };

    for (const QString &field : requiredFields) {
        if (!root.contains(field) || !root.value(field).isString()) {
            if (error != nullptr) {
                *error = (QStringLiteral("manifest missing required field: %1").arg(field)).toStdString();
            }
            return false;
        }
    }

    if (version != nullptr) {
        *version = root.value(QStringLiteral("version")).toString().toStdString();
    }
    if (url != nullptr) {
        *url = root.value(QStringLiteral("url")).toString().toStdString();
    }
    if (sha256 != nullptr) {
        *sha256 = toLowerAscii(root.value(QStringLiteral("sha256")).toString().trimmed().toStdString());
    }
    return true;
}

bool sha256File(const std::filesystem::path &path, std::string *digest) {
    if (digest == nullptr) {
        return false;
    }

    HANDLE file = CreateFileW(path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    bool success = false;

    if (CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) &&
        CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash)) {
        BYTE buffer[8192];
        DWORD bytesRead = 0;
        success = true;
        while (ReadFile(file, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
            if (!CryptHashData(hash, buffer, bytesRead, 0)) {
                success = false;
                break;
            }
        }

        if (success) {
            BYTE hashBytes[32];
            DWORD hashLength = sizeof(hashBytes);
            if (CryptGetHashParam(hash, HP_HASHVAL, hashBytes, &hashLength, 0)) {
                static const char hex[] = "0123456789abcdef";
                std::string out;
                out.reserve(hashLength * 2);
                for (DWORD index = 0; index < hashLength; ++index) {
                    out.push_back(hex[(hashBytes[index] >> 4) & 0x0F]);
                    out.push_back(hex[hashBytes[index] & 0x0F]);
                }
                *digest = out;
            } else {
                success = false;
            }
        }
    }

    if (hash != 0) {
        CryptDestroyHash(hash);
    }
    if (provider != 0) {
        CryptReleaseContext(provider, 0);
    }
    CloseHandle(file);
    return success;
}

std::vector<int> parseVersion(const std::string &version) {
    std::vector<int> parts;
    std::stringstream stream(version);
    std::string token;
    while (std::getline(stream, token, '.')) {
        parts.push_back(std::atoi(token.c_str()));
    }
    while (parts.size() < 3) {
        parts.push_back(0);
    }
    return parts;
}

bool isVersionNewer(const std::string &currentVersion, const std::string &candidateVersion) {
    const auto current = parseVersion(currentVersion);
    const auto candidate = parseVersion(candidateVersion);
    return candidate > current;
}

std::string downloadText(const std::string &url) {
    HINTERNET internet = InternetOpenA("PulseBoostUpdater", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!internet) {
        return {};
    }
    HINTERNET request = InternetOpenUrlA(internet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!request) {
        InternetCloseHandle(internet);
        return {};
    }

    std::string body;
    char buffer[4096];
    DWORD bytesRead = 0;
    while (InternetReadFile(request, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        body.append(buffer, buffer + bytesRead);
    }

    InternetCloseHandle(request);
    InternetCloseHandle(internet);
    return body;
}

}  // namespace

struct AutoUpdater::Impl {
    bool updateAvailable = false;
    bool manifestTrusted = false;
    std::string downloadUrl;
    std::string latestVersion;
    std::string expectedSha256;
    std::string lastError;
    std::filesystem::path downloadedInstallerPath;
};

AutoUpdater::AutoUpdater() : pImpl(new Impl()) {}

AutoUpdater::~AutoUpdater() {
    delete pImpl;
}

bool AutoUpdater::checkForUpdates(const std::string &currentVersion) {
    const char *envUrl = std::getenv("PULSEBOOST_UPDATE_MANIFEST_URL");
    const std::string manifestUrl = envUrl != nullptr && std::string(envUrl).size() > 0
                                        ? std::string(envUrl)
                                        : "https://api.heladevstudio.com/v1/updates/manifest.json";

    std::cout << "[Updater] Checking for updates... Current: " << currentVersion << "\n";
    pImpl->lastError.clear();
    pImpl->manifestTrusted = false;
    pImpl->expectedSha256.clear();
    pImpl->downloadedInstallerPath.clear();

    const std::string manifest = downloadText(manifestUrl);
    if (manifest.empty()) {
        pImpl->updateAvailable = false;
        pImpl->lastError = "manifest-download-failed";
        return false;
    }

    std::string manifestError;
    if (!parseManifest(manifest, &pImpl->latestVersion, &pImpl->downloadUrl, &pImpl->expectedSha256, &manifestError)) {
        std::cerr << "[Updater] Manifest validation failed: " << manifestError << "\n";
        pImpl->updateAvailable = false;
        pImpl->latestVersion.clear();
        pImpl->downloadUrl.clear();
        pImpl->expectedSha256.clear();
        pImpl->lastError = manifestError;
        return false;
    }

    pImpl->manifestTrusted = !pImpl->expectedSha256.empty() && pImpl->expectedSha256.size() == 64;
    if (!pImpl->manifestTrusted) {
        pImpl->updateAvailable = false;
        pImpl->lastError = "manifest-missing-valid-sha256";
        return false;
    }

    pImpl->updateAvailable =
        !pImpl->latestVersion.empty() && !pImpl->downloadUrl.empty() && isVersionNewer(currentVersion, pImpl->latestVersion);
    return pImpl->updateAvailable;
}

bool AutoUpdater::downloadUpdate() {
    pImpl->lastError.clear();
    if (!pImpl->updateAvailable || pImpl->downloadUrl.empty() || !pImpl->manifestTrusted) {
        pImpl->lastError = "update-download-preconditions-not-met";
        return false;
    }

    std::error_code error;
    const std::filesystem::path cacheDir = std::filesystem::temp_directory_path(error) / "PulseBoostAI";
    std::filesystem::create_directories(cacheDir, error);
    if (error) {
        pImpl->lastError = "cache-directory-unavailable";
        return false;
    }

    const std::filesystem::path installerPath = cacheDir / "PulseBoostAI-Update.exe";
    std::cout << "[Updater] Downloading from " << pImpl->downloadUrl << "...\n";
    const HRESULT status = URLDownloadToFileA(nullptr, pImpl->downloadUrl.c_str(), installerPath.string().c_str(), 0, nullptr);
    if (FAILED(status)) {
        pImpl->lastError = "installer-download-failed";
        return false;
    }

    std::string downloadedSha256;
    if (!sha256File(installerPath, &downloadedSha256)) {
        std::filesystem::remove(installerPath, error);
        pImpl->lastError = "installer-hash-failed";
        return false;
    }

    if (downloadedSha256 != toLowerAscii(pImpl->expectedSha256)) {
        std::filesystem::remove(installerPath, error);
        pImpl->lastError = "installer-checksum-mismatch";
        return false;
    }

    pImpl->downloadedInstallerPath = installerPath;
    return true;
}

bool AutoUpdater::installAndRestart() {
    if (!pImpl->updateAvailable || pImpl->downloadedInstallerPath.empty()) {
        pImpl->lastError = "installer-not-ready";
        return false;
    }

    std::cout << "[Updater] Launching installer...\n";
    const HINSTANCE launched = ShellExecuteA(nullptr,
                                             "open",
                                             pImpl->downloadedInstallerPath.string().c_str(),
                                             "/SILENT",
                                             nullptr,
                                             SW_SHOWNORMAL);
    const bool launchedOk = reinterpret_cast<std::intptr_t>(launched) > 32;
    if (!launchedOk) {
        pImpl->lastError = "installer-launch-failed";
    }
    return launchedOk;
}


bool AutoUpdater::updateAvailable() const {
    return pImpl->updateAvailable;
}

std::string AutoUpdater::latestVersion() const {
    return pImpl->latestVersion;
}

std::string AutoUpdater::downloadUrl() const {
    return pImpl->downloadUrl;
}

std::string AutoUpdater::expectedSha256() const {
    return pImpl->expectedSha256;
}

bool AutoUpdater::manifestTrusted() const {
    return pImpl->manifestTrusted;
}

std::string AutoUpdater::lastError() const {
    return pImpl->lastError;
}

bool AutoUpdater::hasDownloadedInstaller() const {
    return !pImpl->downloadedInstallerPath.empty();
}
}  // namespace pulseboost
