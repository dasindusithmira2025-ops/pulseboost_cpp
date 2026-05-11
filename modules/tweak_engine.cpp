#include "PulseBoostAI/modules/tweak_engine.hpp"

#include <Windows.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "PulseBoostAI/common/windows_utils.hpp"
#include "PulseBoostAI/core/registry_optimizer.hpp"
#include "PulseBoostAI/core/service_manager.hpp"

namespace pulseboost {

namespace {

constexpr wchar_t kGraphicsDriversKey[] = L"SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers";
constexpr wchar_t kThemesPersonalizeKey[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
constexpr wchar_t kMouseKey[] = L"Control Panel\\Mouse";
constexpr wchar_t kPowerThrottlingKey[] = L"SYSTEM\\CurrentControlSet\\Control\\Power\\PowerThrottling";
constexpr wchar_t kDataCollectionKey[] = L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection";
constexpr wchar_t kSystemPolicyKey[] = L"SOFTWARE\\Policies\\Microsoft\\Windows\\System";
constexpr wchar_t kMultimediaSystemProfileKey[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile";
constexpr wchar_t kGameBarKey[] = L"SOFTWARE\\Microsoft\\GameBar";
constexpr wchar_t kExplorerAdvancedKey[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
constexpr wchar_t kDesktopWindowMetricsKey[] = L"Control Panel\\Desktop\\WindowMetrics";
constexpr wchar_t kDwmKey[] = L"SOFTWARE\\Microsoft\\Windows\\DWM";
constexpr wchar_t kSiufRulesKey[] = L"SOFTWARE\\Microsoft\\Siuf\\Rules";
constexpr wchar_t kAdvertisingInfoKey[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdvertisingInfo";
constexpr wchar_t kPschedKey[] = L"SOFTWARE\\Policies\\Microsoft\\Windows\\Psched";

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::filesystem::path tweakBackupDirectory() {
    const char *appData = std::getenv("APPDATA");
    std::filesystem::path base = appData != nullptr ? std::filesystem::path(appData) : std::filesystem::path("data");
    base /= "PulseBoostAI";
    base /= "backups";
    base /= "tweaks";
    std::error_code error;
    std::filesystem::create_directories(base, error);
    return base;
}

std::filesystem::path tweakBackupPath(const std::string &id) {
    return tweakBackupDirectory() / (id + ".json");
}

bool writeBackup(const std::string &id, const QJsonObject &backup) {
    const auto path = tweakBackupPath(id);
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output << QJsonDocument(backup).toJson(QJsonDocument::Compact).toStdString();
    return output.good();
}

std::optional<QJsonObject> readBackup(const std::string &id) {
    const auto path = tweakBackupPath(id);
    std::ifstream input(path);
    if (!input.is_open()) {
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    const QByteArray json = QByteArray::fromStdString(buffer.str());
    QJsonParseError error {};
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }
    return document.object();
}

bool backupExists(const std::string &id) {
    std::error_code error;
    return std::filesystem::exists(tweakBackupPath(id), error) && !error;
}

std::wstring rootName(HKEY rootKey) {
    return rootKey == HKEY_CURRENT_USER ? L"HKCU" : L"HKLM";
}

HKEY rootKeyFromName(const std::string &name) {
    return toLowerAscii(name) == "hkcu" ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
}

std::optional<std::uint32_t> readDwordValue(HKEY rootKey, const std::wstring &subKey, const std::wstring &valueName) {
    HKEY keyHandle = nullptr;
    if (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_QUERY_VALUE, &keyHandle) != ERROR_SUCCESS) {
        return std::nullopt;
    }

    DWORD type = 0;
    DWORD value = 0;
    DWORD size = sizeof(value);
    const LONG status = RegQueryValueExW(keyHandle,
                                         valueName.c_str(),
                                         nullptr,
                                         &type,
                                         reinterpret_cast<BYTE *>(&value),
                                         &size);
    RegCloseKey(keyHandle);
    if (status != ERROR_SUCCESS || type != REG_DWORD) {
        return std::nullopt;
    }
    return value;
}

std::optional<std::wstring> readStringValue(HKEY rootKey, const std::wstring &subKey, const std::wstring &valueName) {
    HKEY keyHandle = nullptr;
    if (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_QUERY_VALUE, &keyHandle) != ERROR_SUCCESS) {
        return std::nullopt;
    }

    DWORD type = 0;
    DWORD bytes = 0;
    LONG status = RegQueryValueExW(keyHandle, valueName.c_str(), nullptr, &type, nullptr, &bytes);
    if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        RegCloseKey(keyHandle);
        return std::nullopt;
    }

    std::wstring value(bytes / sizeof(wchar_t), L'\0');
    status = RegQueryValueExW(keyHandle,
                              valueName.c_str(),
                              nullptr,
                              &type,
                              reinterpret_cast<BYTE *>(value.data()),
                              &bytes);
    RegCloseKey(keyHandle);
    if (status != ERROR_SUCCESS) {
        return std::nullopt;
    }

    if (!value.empty() && value.back() == L'\0') {
        value.pop_back();
    }
    return value;
}

bool ensureKey(HKEY rootKey, const std::wstring &subKey, REGSAM access, HKEY *keyHandle) {
    return RegCreateKeyExW(rootKey,
                           subKey.c_str(),
                           0,
                           nullptr,
                           REG_OPTION_NON_VOLATILE,
                           access,
                           nullptr,
                           keyHandle,
                           nullptr) == ERROR_SUCCESS;
}

bool writeDwordValue(HKEY rootKey, const std::wstring &subKey, const std::wstring &valueName, std::uint32_t value) {
    HKEY keyHandle = nullptr;
    if (!ensureKey(rootKey, subKey, KEY_SET_VALUE, &keyHandle)) {
        return false;
    }

    const DWORD rawValue = value;
    const bool success = RegSetValueExW(keyHandle,
                                        valueName.c_str(),
                                        0,
                                        REG_DWORD,
                                        reinterpret_cast<const BYTE *>(&rawValue),
                                        sizeof(rawValue)) == ERROR_SUCCESS;
    RegCloseKey(keyHandle);
    return success;
}

bool writeStringValue(HKEY rootKey, const std::wstring &subKey, const std::wstring &valueName, const std::wstring &value) {
    HKEY keyHandle = nullptr;
    if (!ensureKey(rootKey, subKey, KEY_SET_VALUE, &keyHandle)) {
        return false;
    }

    const DWORD bytes = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    const bool success = RegSetValueExW(keyHandle,
                                        valueName.c_str(),
                                        0,
                                        REG_SZ,
                                        reinterpret_cast<const BYTE *>(value.c_str()),
                                        bytes) == ERROR_SUCCESS;
    RegCloseKey(keyHandle);
    return success;
}

bool deleteValue(HKEY rootKey, const std::wstring &subKey, const std::wstring &valueName) {
    HKEY keyHandle = nullptr;
    if (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_SET_VALUE, &keyHandle) != ERROR_SUCCESS) {
        return false;
    }
    const bool success = RegDeleteValueW(keyHandle, valueName.c_str()) == ERROR_SUCCESS;
    RegCloseKey(keyHandle);
    return success;
}

bool registryValueExists(HKEY rootKey, const std::wstring &subKey, const std::wstring &valueName) {
    HKEY keyHandle = nullptr;
    if (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_QUERY_VALUE, &keyHandle) != ERROR_SUCCESS) {
        return false;
    }
    const LONG status = RegQueryValueExW(keyHandle, valueName.c_str(), nullptr, nullptr, nullptr, nullptr);
    RegCloseKey(keyHandle);
    return status == ERROR_SUCCESS;
}

QJsonObject captureDwordSetting(HKEY rootKey, const std::wstring &subKey, const std::wstring &valueName) {
    QJsonObject entry;
    entry["kind"] = "dword";
    entry["root"] = QString::fromStdWString(rootName(rootKey));
    entry["subKey"] = QString::fromStdWString(subKey);
    entry["name"] = QString::fromStdWString(valueName);
    const auto existing = readDwordValue(rootKey, subKey, valueName);
    entry["exists"] = existing.has_value();
    if (existing.has_value()) {
        entry["value"] = static_cast<qint64>(*existing);
    }
    return entry;
}

QJsonObject captureStringSetting(HKEY rootKey, const std::wstring &subKey, const std::wstring &valueName) {
    QJsonObject entry;
    entry["kind"] = "string";
    entry["root"] = QString::fromStdWString(rootName(rootKey));
    entry["subKey"] = QString::fromStdWString(subKey);
    entry["name"] = QString::fromStdWString(valueName);
    const auto existing = readStringValue(rootKey, subKey, valueName);
    entry["exists"] = existing.has_value();
    if (existing.has_value()) {
        entry["value"] = QString::fromStdWString(*existing);
    }
    return entry;
}

bool restoreRegistryEntry(const QJsonObject &entry) {
    const HKEY rootKey = rootKeyFromName(entry.value("root").toString().toStdString());
    const std::wstring subKey = entry.value("subKey").toString().toStdWString();
    const std::wstring name = entry.value("name").toString().toStdWString();
    const bool exists = entry.value("exists").toBool(false);
    const std::string kind = entry.value("kind").toString().toStdString();
    if (!exists) {
        const bool removed = deleteValue(rootKey, subKey, name);
        return removed || !registryValueExists(rootKey, subKey, name);
    }

    if (kind == "dword") {
        return writeDwordValue(rootKey, subKey, name, entry.value("value").toInt());
    }
    if (kind == "string") {
        return writeStringValue(rootKey, subKey, name, entry.value("value").toString().toStdWString());
    }
    return false;
}

std::optional<ServiceInfo> findService(const ServiceManager &serviceManager, const std::string &name) {
    const auto lowerName = toLowerAscii(name);
    for (const auto &service : serviceManager.enumerateServices(false)) {
        if (toLowerAscii(service.name) == lowerName) {
            return service;
        }
    }
    return std::nullopt;
}

bool configureServiceStartType(const std::string &name, const std::string &startMode) {
    const std::wstring command = L"sc.exe config \"" + toWide(name) + L"\" start= " + toWide(startMode);
    DWORD exitCode = 1;
    return runProcessHidden(command, &exitCode) && exitCode == 0;
}

QJsonObject captureServiceState(const ServiceManager &serviceManager, const std::string &name) {
    QJsonObject entry;
    entry["kind"] = "service";
    entry["name"] = QString::fromStdString(name);
    if (const auto service = findService(serviceManager, name); service.has_value()) {
        entry["exists"] = true;
        entry["startMode"] = QString::fromStdString(service->startMode);
        entry["state"] = QString::fromStdString(service->state);
    } else {
        entry["exists"] = false;
    }
    return entry;
}

std::string normalizeServiceStartMode(const std::string &startMode) {
    const std::string lower = toLowerAscii(startMode);
    if (lower.find("disabled") != std::string::npos) {
        return "disabled";
    }
    if (lower.find("manual") != std::string::npos) {
        return "demand";
    }
    return "auto";
}

bool restoreServiceState(const ServiceManager &serviceManager, const QJsonObject &entry) {
    if (!entry.value("exists").toBool(false)) {
        return true;
    }

    const std::string name = entry.value("name").toString().toStdString();
    const std::string startMode = normalizeServiceStartMode(entry.value("startMode").toString().toStdString());
    bool ok = configureServiceStartType(name, startMode);
    const std::string state = toLowerAscii(entry.value("state").toString().toStdString());
    if (state.find("running") != std::string::npos) {
        ok = serviceManager.startService(name) || ok;
    } else if (state.find("stopped") != std::string::npos) {
        ok = serviceManager.stopService(name) || ok;
    }
    return ok;
}

bool ensureBackupWritten(const std::string &id, const QJsonObject &backup) {
    if (backupExists(id)) {
        return true;
    }
    return writeBackup(id, backup);
}

bool allEntriesRestored(const QJsonArray &entries, const ServiceManager &serviceManager) {
    bool ok = true;
    for (const QJsonValue &value : entries) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject entry = value.toObject();
        const std::string kind = entry.value("kind").toString().toStdString();
        if (kind == "service") {
            ok = restoreServiceState(serviceManager, entry) && ok;
        } else {
            ok = restoreRegistryEntry(entry) && ok;
        }
    }
    return ok;
}

TweakDefinition makeTweak(const std::string &id,
                          const std::string &category,
                          const std::string &name,
                          const std::string &description,
                          const std::string &details,
                          const std::string &impact,
                          const std::string &risk,
                          bool requiresRestart,
                          bool isApplied,
                          bool isApplicable = true,
                          const std::string &notApplicableReason = {}) {
    TweakDefinition tweak;
    tweak.id = id;
    tweak.category = category;
    tweak.name = name;
    tweak.description = description;
    tweak.detailedInfo = details;
    tweak.impact = impact;
    tweak.riskLevel = risk;
    tweak.requiresRestart = requiresRestart;
    tweak.isApplied = isApplied;
    tweak.isApplicable = isApplicable;
    tweak.notApplicableReason = notApplicableReason;
    return tweak;
}

}  // namespace

TweakEngine::TweakEngine(RegistryOptimizer &registryOptimizer, ServiceManager &serviceManager)
    : registryOptimizer_(registryOptimizer), serviceManager_(serviceManager) {}

std::vector<TweakDefinition> TweakEngine::listTweaks() const {
    std::vector<TweakDefinition> tweaks;
    tweaks.reserve(17);

    tweaks.push_back(makeTweak(
        "power_disable_power_throttling",
        "power",
        "Disable Power Throttling",
        "Stops Windows from downclocking foreground workloads aggressively.",
        "HKLM\\SYSTEM\\CurrentControlSet\\Control\\Power\\PowerThrottling -> PowerThrottlingOff=1",
        "high",
        "safe",
        true,
        readDwordValue(HKEY_LOCAL_MACHINE, kPowerThrottlingKey, L"PowerThrottlingOff").value_or(0) == 1));

    tweaks.push_back(makeTweak(
        "windows_disable_transparency",
        "windows",
        "Disable Transparency Effects",
        "Removes acrylic transparency to reduce desktop compositor overhead.",
        "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize -> EnableTransparency=0",
        "low",
        "safe",
        false,
        readDwordValue(HKEY_CURRENT_USER, kThemesPersonalizeKey, L"EnableTransparency").value_or(1) == 0));

    const bool mouseAccelApplied =
        readStringValue(HKEY_CURRENT_USER, kMouseKey, L"MouseSpeed").value_or(L"1") == L"0" &&
        readStringValue(HKEY_CURRENT_USER, kMouseKey, L"MouseThreshold1").value_or(L"6") == L"0" &&
        readStringValue(HKEY_CURRENT_USER, kMouseKey, L"MouseThreshold2").value_or(L"10") == L"0";
    tweaks.push_back(makeTweak(
        "gaming_disable_mouse_acceleration",
        "gaming",
        "Disable Mouse Acceleration",
        "Turns off enhanced pointer precision for predictable aim and raw input feel.",
        "HKCU\\Control Panel\\Mouse -> MouseSpeed/MouseThreshold1/MouseThreshold2 = 0",
        "high",
        "safe",
        false,
        mouseAccelApplied));

    tweaks.push_back(makeTweak(
        "gpu_enable_hardware_gpu_scheduling",
        "gpu",
        "Enable Hardware GPU Scheduling",
        "Lets the GPU scheduler handle frame queueing directly on supported hardware.",
        "HKLM\\SYSTEM\\CurrentControlSet\\Control\\GraphicsDrivers -> HwSchMode=2",
        "high",
        "moderate",
        true,
        readDwordValue(HKEY_LOCAL_MACHINE, kGraphicsDriversKey, L"HwSchMode").value_or(1) == 2));

    tweaks.push_back(makeTweak(
        "network_disable_network_throttling",
        "network",
        "Disable Network Throttling",
        "Removes Windows multimedia network throttling that can hurt game traffic bursts.",
        "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile -> NetworkThrottlingIndex=0xFFFFFFFF",
        "high",
        "moderate",
        true,
        readDwordValue(HKEY_LOCAL_MACHINE, kMultimediaSystemProfileKey, L"NetworkThrottlingIndex").value_or(0) == 0xFFFFFFFFu));

    tweaks.push_back(makeTweak(
        "network_system_responsiveness",
        "network",
        "Set System Responsiveness To 0",
        "Prioritizes foreground latency-sensitive workloads over background multimedia reservation.",
        "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile -> SystemResponsiveness=0",
        "high",
        "moderate",
        true,
        readDwordValue(HKEY_LOCAL_MACHINE, kMultimediaSystemProfileKey, L"SystemResponsiveness").value_or(20) == 0));

    tweaks.push_back(makeTweak(
        "privacy_disable_telemetry",
        "privacy",
        "Disable Windows Telemetry Policy",
        "Cuts back diagnostic data collection on systems where telemetry is not wanted.",
        "HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection -> AllowTelemetry=0",
        "medium",
        "moderate",
        true,
        readDwordValue(HKEY_LOCAL_MACHINE, kDataCollectionKey, L"AllowTelemetry").value_or(3) == 0));

    tweaks.push_back(makeTweak(
        "privacy_disable_activity_history",
        "privacy",
        "Disable Activity History Publishing",
        "Stops Windows from publishing user activity history to Timeline-style services.",
        "HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System -> PublishUserActivities=0",
        "medium",
        "safe",
        true,
        readDwordValue(HKEY_LOCAL_MACHINE, kSystemPolicyKey, L"PublishUserActivities").value_or(1) == 0));

    tweaks.push_back(makeTweak(
        "windows_disable_sysmain",
        "windows",
        "Disable SysMain",
        "Stops the SysMain service when it causes storage thrash on gaming and SSD-first systems.",
        "Service: SysMain -> startup disabled, service stopped",
        "medium",
        "moderate",
        true,
        findService(serviceManager_, "SysMain").has_value() &&
            toLowerAscii(findService(serviceManager_, "SysMain")->startMode).find("disabled") != std::string::npos));

    tweaks.push_back(makeTweak(
        "windows_disable_search",
        "windows",
        "Disable Windows Search Indexing",
        "Stops WSearch to reduce indexing load on systems dedicated to gaming or focused work.",
        "Service: WSearch -> startup disabled, service stopped",
        "medium",
        "moderate",
        true,
        findService(serviceManager_, "WSearch").has_value() &&
            toLowerAscii(findService(serviceManager_, "WSearch")->startMode).find("disabled") != std::string::npos));

    tweaks.push_back(makeTweak(
        "gaming_enable_game_mode",
        "gaming",
        "Enable Windows Game Mode",
        "Turns on the built-in Game Mode policy and auto-enable behavior.",
        "HKCU\\SOFTWARE\\Microsoft\\GameBar -> AllowAutoGameMode=1, AutoGameModeEnabled=1",
        "medium",
        "safe",
        false,
        readDwordValue(HKEY_CURRENT_USER, kGameBarKey, L"AllowAutoGameMode").value_or(0) == 1 &&
            readDwordValue(HKEY_CURRENT_USER, kGameBarKey, L"AutoGameModeEnabled").value_or(0) == 1));

    tweaks.push_back(makeTweak(
        "windows_disable_animations",
        "windows",
        "Disable Window Animations",
        "Turns off MinAnimate to reduce shell animation overhead on desktops focused on responsiveness.",
        "HKCU\\Control Panel\\Desktop\\WindowMetrics -> MinAnimate=0",
        "medium",
        "safe",
        false,
        readStringValue(HKEY_CURRENT_USER, kDesktopWindowMetricsKey, L"MinAnimate").value_or(L"1") == L"0"));

    tweaks.push_back(makeTweak(
        "windows_disable_aero_peek",
        "windows",
        "Disable Aero Peek",
        "Removes peek previews from the desktop compositor to keep transitions lighter.",
        "HKCU\\SOFTWARE\\Microsoft\\Windows\\DWM -> EnableAeroPeek=0",
        "low",
        "safe",
        false,
        readDwordValue(HKEY_CURRENT_USER, kDwmKey, L"EnableAeroPeek").value_or(1) == 0));

    tweaks.push_back(makeTweak(
        "privacy_disable_feedback_notifications",
        "privacy",
        "Disable Feedback Notifications",
        "Stops Windows from prompting for periodic feedback after use sessions.",
        "HKCU\\SOFTWARE\\Microsoft\\Siuf\\Rules -> NumberOfSIUFInPeriod=0",
        "low",
        "safe",
        false,
        readDwordValue(HKEY_CURRENT_USER, kSiufRulesKey, L"NumberOfSIUFInPeriod").value_or(1) == 0));

    tweaks.push_back(makeTweak(
        "privacy_disable_advertising_id",
        "privacy",
        "Disable Advertising ID",
        "Turns off the per-user advertising identifier used for app personalization.",
        "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdvertisingInfo -> Enabled=0",
        "medium",
        "safe",
        false,
        readDwordValue(HKEY_CURRENT_USER, kAdvertisingInfoKey, L"Enabled").value_or(1) == 0));

    tweaks.push_back(makeTweak(
        "network_remove_reserved_bandwidth",
        "network",
        "Remove Reserved QoS Bandwidth",
        "Sets NonBestEffortLimit to 0 so Windows does not reserve background QoS bandwidth for itself.",
        "HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Psched -> NonBestEffortLimit=0",
        "medium",
        "moderate",
        true,
        readDwordValue(HKEY_LOCAL_MACHINE, kPschedKey, L"NonBestEffortLimit").value_or(20) == 0));

    tweaks.push_back(makeTweak(
        "windows_disable_taskbar_animations",
        "windows",
        "Disable Taskbar Animations",
        "Removes shell animation overhead from taskbar transitions.",
        "HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced -> TaskbarAnimations=0",
        "low",
        "safe",
        false,
        readDwordValue(HKEY_CURRENT_USER, kExplorerAdvancedKey, L"TaskbarAnimations").value_or(1) == 0));

    return tweaks;
}

int TweakEngine::appliedCount() const {
    const auto tweaks = listTweaks();
    return static_cast<int>(std::count_if(tweaks.begin(), tweaks.end(), [](const TweakDefinition &tweak) {
        return tweak.isApplied && tweak.isApplicable;
    }));
}

TweakActionResult TweakEngine::applyTweak(const std::string &id) const {
    TweakActionResult result;
    result.id = id;

    auto currentTweaks = listTweaks();
    const auto match = std::find_if(currentTweaks.begin(), currentTweaks.end(), [&](const TweakDefinition &tweak) {
        return tweak.id == id;
    });
    if (match == currentTweaks.end()) {
        result.error = "unknown-tweak";
        return result;
    }

    result.requiresRestart = match->requiresRestart;
    if (match->isApplied) {
        result.success = true;
        result.isApplied = true;
        return result;
    }

    bool ok = false;
    if (id == "power_disable_power_throttling") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_LOCAL_MACHINE, kPowerThrottlingKey, L"PowerThrottlingOff")};
        ok = ensureBackupWritten(id, backup) && writeDwordValue(HKEY_LOCAL_MACHINE, kPowerThrottlingKey, L"PowerThrottlingOff", 1);
    } else if (id == "windows_disable_transparency") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_CURRENT_USER, kThemesPersonalizeKey, L"EnableTransparency")};
        ok = ensureBackupWritten(id, backup) && writeDwordValue(HKEY_CURRENT_USER, kThemesPersonalizeKey, L"EnableTransparency", 0);
    } else if (id == "gaming_disable_mouse_acceleration") {
        QJsonArray entries;
        entries.append(captureStringSetting(HKEY_CURRENT_USER, kMouseKey, L"MouseSpeed"));
        entries.append(captureStringSetting(HKEY_CURRENT_USER, kMouseKey, L"MouseThreshold1"));
        entries.append(captureStringSetting(HKEY_CURRENT_USER, kMouseKey, L"MouseThreshold2"));
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = entries;
        ok = ensureBackupWritten(id, backup) &&
            writeStringValue(HKEY_CURRENT_USER, kMouseKey, L"MouseSpeed", L"0") &&
            writeStringValue(HKEY_CURRENT_USER, kMouseKey, L"MouseThreshold1", L"0") &&
            writeStringValue(HKEY_CURRENT_USER, kMouseKey, L"MouseThreshold2", L"0");
    } else if (id == "gpu_enable_hardware_gpu_scheduling") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_LOCAL_MACHINE, kGraphicsDriversKey, L"HwSchMode")};
        ok = ensureBackupWritten(id, backup) && writeDwordValue(HKEY_LOCAL_MACHINE, kGraphicsDriversKey, L"HwSchMode", 2);
    } else if (id == "network_disable_network_throttling") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_LOCAL_MACHINE, kMultimediaSystemProfileKey, L"NetworkThrottlingIndex")};
        ok = ensureBackupWritten(id, backup) &&
            writeDwordValue(HKEY_LOCAL_MACHINE, kMultimediaSystemProfileKey, L"NetworkThrottlingIndex", 0xFFFFFFFFu);
    } else if (id == "network_system_responsiveness") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_LOCAL_MACHINE, kMultimediaSystemProfileKey, L"SystemResponsiveness")};
        ok = ensureBackupWritten(id, backup) &&
            writeDwordValue(HKEY_LOCAL_MACHINE, kMultimediaSystemProfileKey, L"SystemResponsiveness", 0);
    } else if (id == "privacy_disable_telemetry") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_LOCAL_MACHINE, kDataCollectionKey, L"AllowTelemetry")};
        ok = ensureBackupWritten(id, backup) && writeDwordValue(HKEY_LOCAL_MACHINE, kDataCollectionKey, L"AllowTelemetry", 0);
    } else if (id == "privacy_disable_activity_history") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_LOCAL_MACHINE, kSystemPolicyKey, L"PublishUserActivities")};
        ok = ensureBackupWritten(id, backup) && writeDwordValue(HKEY_LOCAL_MACHINE, kSystemPolicyKey, L"PublishUserActivities", 0);
    } else if (id == "windows_disable_sysmain") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureServiceState(serviceManager_, "SysMain")};
        ok = ensureBackupWritten(id, backup) && configureServiceStartType("SysMain", "disabled");
        ok = serviceManager_.stopService("SysMain") || ok;
    } else if (id == "windows_disable_search") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureServiceState(serviceManager_, "WSearch")};
        ok = ensureBackupWritten(id, backup) && configureServiceStartType("WSearch", "disabled");
        ok = serviceManager_.stopService("WSearch") || ok;
    } else if (id == "gaming_enable_game_mode") {
        QJsonArray entries;
        entries.append(captureDwordSetting(HKEY_CURRENT_USER, kGameBarKey, L"AllowAutoGameMode"));
        entries.append(captureDwordSetting(HKEY_CURRENT_USER, kGameBarKey, L"AutoGameModeEnabled"));
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = entries;
        ok = ensureBackupWritten(id, backup) &&
            writeDwordValue(HKEY_CURRENT_USER, kGameBarKey, L"AllowAutoGameMode", 1) &&
            writeDwordValue(HKEY_CURRENT_USER, kGameBarKey, L"AutoGameModeEnabled", 1);
    } else if (id == "windows_disable_animations") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureStringSetting(HKEY_CURRENT_USER, kDesktopWindowMetricsKey, L"MinAnimate")};
        ok = ensureBackupWritten(id, backup) && writeStringValue(HKEY_CURRENT_USER, kDesktopWindowMetricsKey, L"MinAnimate", L"0");
    } else if (id == "windows_disable_aero_peek") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_CURRENT_USER, kDwmKey, L"EnableAeroPeek")};
        ok = ensureBackupWritten(id, backup) && writeDwordValue(HKEY_CURRENT_USER, kDwmKey, L"EnableAeroPeek", 0);
    } else if (id == "privacy_disable_feedback_notifications") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_CURRENT_USER, kSiufRulesKey, L"NumberOfSIUFInPeriod")};
        ok = ensureBackupWritten(id, backup) && writeDwordValue(HKEY_CURRENT_USER, kSiufRulesKey, L"NumberOfSIUFInPeriod", 0);
    } else if (id == "privacy_disable_advertising_id") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_CURRENT_USER, kAdvertisingInfoKey, L"Enabled")};
        ok = ensureBackupWritten(id, backup) && writeDwordValue(HKEY_CURRENT_USER, kAdvertisingInfoKey, L"Enabled", 0);
    } else if (id == "network_remove_reserved_bandwidth") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_LOCAL_MACHINE, kPschedKey, L"NonBestEffortLimit")};
        ok = ensureBackupWritten(id, backup) && writeDwordValue(HKEY_LOCAL_MACHINE, kPschedKey, L"NonBestEffortLimit", 0);
    } else if (id == "windows_disable_taskbar_animations") {
        QJsonObject backup;
        backup["id"] = QString::fromStdString(id);
        backup["entries"] = QJsonArray {captureDwordSetting(HKEY_CURRENT_USER, kExplorerAdvancedKey, L"TaskbarAnimations")};
        ok = ensureBackupWritten(id, backup) && writeDwordValue(HKEY_CURRENT_USER, kExplorerAdvancedKey, L"TaskbarAnimations", 0);
    }

    result.success = ok;
    result.isApplied = ok ? true : match->isApplied;
    if (!ok) {
        result.error = "apply-failed";
    }
    return result;
}

std::vector<TweakActionResult> TweakEngine::applySafeTweaks() const {
    std::vector<TweakActionResult> results;
    for (const auto &tweak : listTweaks()) {
        if (!tweak.isApplicable || tweak.isApplied || tweak.riskLevel != "safe") {
            continue;
        }
        results.push_back(applyTweak(tweak.id));
    }
    return results;
}

std::vector<TweakActionResult> TweakEngine::applyHighImpactTweaks() const {
    std::vector<TweakActionResult> results;
    for (const auto &tweak : listTweaks()) {
        if (!tweak.isApplicable || tweak.isApplied || tweak.impact != "high") {
            continue;
        }
        results.push_back(applyTweak(tweak.id));
    }
    return results;
}

std::vector<TweakActionResult> TweakEngine::revertAllTweaks() const {
    std::vector<TweakActionResult> results;
    for (const auto &tweak : listTweaks()) {
        if (!tweak.isApplied) {
            continue;
        }
        results.push_back(revertTweak(tweak.id));
    }
    return results;
}

TweakActionResult TweakEngine::revertTweak(const std::string &id) const {
    TweakActionResult result;
    result.id = id;

    const auto tweaks = listTweaks();
    const auto match = std::find_if(tweaks.begin(), tweaks.end(), [&](const TweakDefinition &tweak) {
        return tweak.id == id;
    });
    if (match == tweaks.end()) {
        result.error = "unknown-tweak";
        return result;
    }

    result.requiresRestart = match->requiresRestart;
    const auto backup = readBackup(id);
    if (!backup.has_value()) {
        result.success = !match->isApplied;
        result.isApplied = false;
        if (!result.success) {
            result.error = "missing-backup";
        }
        return result;
    }

    const bool ok = allEntriesRestored(backup->value("entries").toArray(), serviceManager_);
    result.success = ok;
    result.isApplied = false;
    if (!ok) {
        result.error = "revert-failed";
    }
    return result;
}

}  // namespace pulseboost

