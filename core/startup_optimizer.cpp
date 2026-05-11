#include "PulseBoostAI/core/startup_optimizer.hpp"

#include <Windows.h>
#include <ShlObj.h>

#include <cctype>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_set>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

namespace {

constexpr wchar_t kRunSubKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
const std::filesystem::path kStartupStateDirectory = std::filesystem::path("logs") / "startup_state";

int estimateImpact(const std::string &command) {
    std::string lower = command;
    for (char &character : lower) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }

    if (lower.find("docker") != std::string::npos) {
        return 85;
    }
    if (lower.find("steam") != std::string::npos || lower.find("epic") != std::string::npos) {
        return 75;
    }
    if (lower.find("discord") != std::string::npos || lower.find("teams") != std::string::npos) {
        return 55;
    }
    if (lower.find("update") != std::string::npos || lower.find("updater") != std::string::npos) {
        return 35;
    }
    return 45;
}

std::string lowerAscii(std::string value) {
    for (char &character : value) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

std::wstring decodeRegistryString(const BYTE *data, DWORD bytes) {
    if (data == nullptr || bytes < sizeof(wchar_t)) {
        return {};
    }

    const auto *raw = reinterpret_cast<const wchar_t *>(data);
    std::size_t count = static_cast<std::size_t>(bytes / sizeof(wchar_t));
    if (count == 0) {
        return {};
    }

    if (raw[count - 1] == L'\0') {
        --count;
    } else {
        count = wcsnlen(raw, count);
    }

    if (count == 0) {
        return {};
    }

    return std::wstring(raw, raw + count);
}

bool isRegistryLocation(const std::string &location) {
    return location.find("Registry") != std::string::npos;
}

bool isCurrentUserLocation(const std::string &location) {
    return location.find("Current User") != std::string::npos;
}

std::string sanitizeStateFileName(const std::string &name) {
    std::string safe;
    safe.reserve(name.size());
    for (const unsigned char ch : name) {
        if (std::isalnum(ch)) {
            safe.push_back(static_cast<char>(std::tolower(ch)));
        } else {
            safe.push_back('_');
        }
    }
    if (safe.empty()) {
        safe = "startup_item";
    }
    return safe;
}

std::filesystem::path startupStatePath(const std::string &name) {
    return kStartupStateDirectory / (sanitizeStateFileName(name) + ".state");
}

bool persistDisabledStartupItem(const StartupItem &item) {
    std::error_code error;
    std::filesystem::create_directories(kStartupStateDirectory, error);
    if (error) {
        return false;
    }

    std::ofstream output(startupStatePath(item.name), std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << "name=" << item.name << '\n';
    output << "command=" << item.command << '\n';
    output << "location=" << item.location << '\n';
    output << "impactScore=" << item.impactScore << '\n';
    return true;
}

std::optional<StartupItem> loadDisabledStartupItem(const std::string &name) {
    std::ifstream input(startupStatePath(name));
    if (!input.is_open()) {
        return std::nullopt;
    }

    StartupItem item;
    item.enabled = false;
    std::string line;
    while (std::getline(input, line)) {
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        if (key == "name") {
            item.name = value;
        } else if (key == "command") {
            item.command = value;
        } else if (key == "location") {
            item.location = value;
        } else if (key == "impactScore") {
            item.impactScore = std::atoi(value.c_str());
        }
    }

    if (item.name.empty() || item.command.empty() || item.location.empty()) {
        return std::nullopt;
    }
    return item;
}

void appendDisabledStartupItems(std::vector<StartupItem> &items) {
    std::error_code error;
    if (!std::filesystem::exists(kStartupStateDirectory, error) || error) {
        return;
    }

    std::unordered_set<std::string> knownNames;
    for (const auto &item : items) {
        knownNames.insert(lowerAscii(item.name));
    }

    for (const auto &entry : std::filesystem::directory_iterator(kStartupStateDirectory, std::filesystem::directory_options::skip_permission_denied, error)) {
        if (error) {
            error.clear();
            continue;
        }
        if (!entry.is_regular_file(error) || entry.path().extension() != ".state") {
            error.clear();
            continue;
        }

        const auto disabledItem = loadDisabledStartupItem(entry.path().stem().string());
        if (!disabledItem.has_value()) {
            continue;
        }
        if (knownNames.contains(lowerAscii(disabledItem->name))) {
            continue;
        }
        items.push_back(*disabledItem);
    }
}

void clearDisabledStartupState(const std::string &name) {
    std::error_code error;
    std::filesystem::remove(startupStatePath(name), error);
}

void enumerateRunKey(std::vector<StartupItem> &items, HKEY rootKey, const std::string &location) {
    HKEY keyHandle = nullptr;
    if (RegOpenKeyExW(rootKey, kRunSubKey, 0, KEY_READ, &keyHandle) != ERROR_SUCCESS) {
        return;
    }

    DWORD valueIndex = 0;
    while (true) {
        wchar_t valueName[512] {};
        BYTE valueData[4096] {};
        DWORD valueNameLength = static_cast<DWORD>(std::size(valueName));
        DWORD valueDataLength = static_cast<DWORD>(std::size(valueData));
        DWORD type = 0;

        const LSTATUS status = RegEnumValueW(keyHandle,
                                             valueIndex,
                                             valueName,
                                             &valueNameLength,
                                             nullptr,
                                             &type,
                                             valueData,
                                             &valueDataLength);

        if (status == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (status == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
            const std::wstring commandWide = decodeRegistryString(valueData, valueDataLength);
            if (!commandWide.empty()) {
                const std::string command = fromWide(commandWide);
                items.push_back(StartupItem {
                    .name = fromWide(valueName),
                    .command = command,
                    .location = location,
                    .enabled = true,
                    .impactScore = estimateImpact(command)
                });
            }
        }

        ++valueIndex;
    }

    RegCloseKey(keyHandle);
}

void enumerateStartupFolder(std::vector<StartupItem> &items, int csidl, const std::string &location) {
    wchar_t pathBuffer[MAX_PATH] {};
    if (SHGetFolderPathW(nullptr, csidl, nullptr, SHGFP_TYPE_CURRENT, pathBuffer) != S_OK) {
        return;
    }

    const std::filesystem::path folder(pathBuffer);
    std::error_code error;
    if (!std::filesystem::exists(folder, error) || error) {
        return;
    }

    for (const auto &entry : std::filesystem::directory_iterator(folder, std::filesystem::directory_options::skip_permission_denied, error)) {
        if (error || !entry.is_regular_file(error)) {
            error.clear();
            continue;
        }
        if (entry.path().extension() == ".disabled") {
            continue;
        }

        items.push_back(StartupItem {.name = entry.path().filename().string(),
                                     .command = entry.path().string(),
                                     .location = location,
                                     .enabled = true,
                                     .impactScore = 30});
    }
}

}  // namespace

StartupOptimizer::StartupOptimizer(RegistryOptimizer &registryOptimizer) : registryOptimizer_(registryOptimizer) {}

std::vector<StartupItem> StartupOptimizer::scanStartupItems() const {
    std::vector<StartupItem> items;
    try {
        enumerateRunKey(items, HKEY_CURRENT_USER, "Registry (Current User)");
        enumerateRunKey(items, HKEY_LOCAL_MACHINE, "Registry (Machine)");
        enumerateStartupFolder(items, CSIDL_STARTUP, "Startup Folder (Current User)");
        enumerateStartupFolder(items, CSIDL_COMMON_STARTUP, "Startup Folder (All Users)");
        appendDisabledStartupItems(items);
    } catch (...) {
    }
    return items;
}

bool StartupOptimizer::disableStartupItem(const StartupItem &item, const std::filesystem::path &backupDirectory) const {
    if (!item.enabled) {
        return true;
    }

    const auto backup = registryOptimizer_.backupRunKeys(backupDirectory);
    if (!backup.has_value()) {
        return false;
    }
    if (!persistDisabledStartupItem(item)) {
        return false;
    }

    bool success = false;
    if (isRegistryLocation(item.location)) {
        success = registryOptimizer_.deleteRunValue(kRunSubKey, toWide(item.name), isCurrentUserLocation(item.location));
    } else {
        std::error_code error;
        const std::filesystem::path target(item.command);
        const auto disabledPath = target.parent_path() / (target.filename().string() + ".disabled");
        std::filesystem::rename(target, disabledPath, error);
        success = !error;
    }

    if (!success) {
        clearDisabledStartupState(item.name);
    }
    return success;
}

bool StartupOptimizer::enableStartupItem(const StartupItem &item) const {
    if (item.enabled) {
        return true;
    }

    const auto persistedItem = loadDisabledStartupItem(item.name);
    const StartupItem source = persistedItem.value_or(item);

    bool success = false;
    if (isRegistryLocation(source.location)) {
        success = registryOptimizer_.setRunValue(kRunSubKey,
                                                toWide(source.name),
                                                toWide(source.command),
                                                isCurrentUserLocation(source.location));
    } else {
        std::error_code error;
        const std::filesystem::path target(source.command);
        const auto disabledPath = target.parent_path() / (target.filename().string() + ".disabled");
        if (std::filesystem::exists(disabledPath, error)) {
            std::filesystem::rename(disabledPath, target, error);
            success = !error;
        } else {
            success = std::filesystem::exists(target, error) && !error;
        }
    }

    if (success) {
        clearDisabledStartupState(source.name);
    }
    return success;
}

bool StartupOptimizer::delayStartupItem(const StartupItem &item, int delaySeconds) const {
    if (item.command.empty() || delaySeconds < 1) {
        return false;
    }

    auto sanitize = [](std::wstring value) {
        for (wchar_t &ch : value) {
            if (ch == L'"' || ch == L'\r' || ch == L'\n' || ch == L'\t') {
                ch = L' ';
            }
        }
        return value;
    };

    const std::wstring taskName = sanitize(L"PulseBoost-" + toWide(item.name));
    const std::wstring targetCommand = sanitize(toWide(item.command));

    const int minutes = delaySeconds / 60;
    std::wstring delay = L"0000:30";
    if (minutes > 0) {
        wchar_t buffer[16] {};
        swprintf_s(buffer, L"000%d:00", minutes);
        delay = buffer;
    }

    const std::wstring command = L"schtasks.exe /Create /F /SC ONLOGON /DELAY " + delay + L" /TN \"" + taskName +
                                 L"\" /TR \"" + targetCommand + L"\"";
    DWORD exitCode = 1;
    return runProcessHidden(command, &exitCode) && exitCode == 0;
}

bool StartupOptimizer::scheduleTask(const std::string &taskId, const std::string &type, int intervalHours) const {
    if (taskId.empty() || intervalHours < 1) {
        return false;
    }

    auto sanitize = [](std::wstring value) {
        for (wchar_t &ch : value) {
            if (ch == L'"' || ch == L'\r' || ch == L'\n' || ch == L'\t') {
                ch = L' ';
            }
        }
        return value;
    };

    std::wstring cliSwitch = L"--scan";
    std::string normalizedType = type;
    for (char &character : normalizedType) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    if (normalizedType.find("junk") != std::string::npos || normalizedType.find("clean") != std::string::npos) {
        cliSwitch = L"--clean";
    } else if (normalizedType.find("ram") != std::string::npos) {
        cliSwitch = L"--optimize-ram";
    } else if (normalizedType.find("startup") != std::string::npos) {
        cliSwitch = L"--status";
    } else if (normalizedType.find("network") != std::string::npos || normalizedType.find("dns") != std::string::npos) {
        cliSwitch = L"--flush-dns";
    }

    wchar_t executablePath[MAX_PATH] {};
    if (GetModuleFileNameW(nullptr, executablePath, static_cast<DWORD>(std::size(executablePath))) == 0) {
        return false;
    }

    const std::wstring taskName = sanitize(L"PulseBoost-" + toWide(taskId));
    const std::wstring taskTarget = sanitize(std::wstring(L"\"") + executablePath + L"\" " + cliSwitch);
    const int safeInterval = std::clamp(intervalHours, 1, 720);

    const std::wstring command = L"schtasks.exe /Create /F /SC HOURLY /MO " +
                                 std::to_wstring(safeInterval) +
                                 L" /TN \"" + taskName +
                                 L"\" /TR \"" + taskTarget +
                                 L"\"";

    DWORD exitCode = 1;
    return runProcessHidden(command, &exitCode) && exitCode == 0;
}
}  // namespace pulseboost

