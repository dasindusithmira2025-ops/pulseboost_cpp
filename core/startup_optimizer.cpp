#include "PulseBoostAI/core/startup_optimizer.hpp"

#include <Windows.h>
#include <ShlObj.h>

#include <cctype>
#include <filesystem>
#include <string>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

namespace {

constexpr wchar_t kRunSubKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

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

void enumerateRunKey(std::vector<StartupItem> &items, HKEY rootKey, const std::string &location) {
    HKEY keyHandle = nullptr;
    if (RegOpenKeyExW(rootKey, kRunSubKey, 0, KEY_READ, &keyHandle) != ERROR_SUCCESS) {
        return;
    }

    DWORD valueIndex = 0;
    while (true) {
        wchar_t valueName[512] {};
        BYTE valueData[2048] {};
        DWORD valueNameLength = static_cast<DWORD>(std::size(valueName));
        DWORD valueDataLength = static_cast<DWORD>(std::size(valueData));
        DWORD type = 0;
        const LSTATUS status = RegEnumValueW(keyHandle, valueIndex, valueName, &valueNameLength, nullptr, &type, valueData,
                                             &valueDataLength);
        if (status == ERROR_NO_MORE_ITEMS) {
            break;
        }
        if (status == ERROR_SUCCESS && type == REG_SZ) {
            const std::wstring command(reinterpret_cast<wchar_t *>(valueData), valueDataLength / sizeof(wchar_t) - 1);
            items.push_back(StartupItem {.name = fromWide(valueName),
                                         .command = fromWide(command),
                                         .location = location,
                                         .enabled = true,
                                         .impactScore = estimateImpact(fromWide(command))});
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
    for (const auto &entry : std::filesystem::directory_iterator(folder, error)) {
        if (error || !entry.is_regular_file(error)) {
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
    enumerateRunKey(items, HKEY_CURRENT_USER, "Registry (Current User)");
    enumerateRunKey(items, HKEY_LOCAL_MACHINE, "Registry (Machine)");
    enumerateStartupFolder(items, CSIDL_STARTUP, "Startup Folder (Current User)");
    enumerateStartupFolder(items, CSIDL_COMMON_STARTUP, "Startup Folder (All Users)");
    return items;
}

bool StartupOptimizer::disableStartupItem(const StartupItem &item, const std::filesystem::path &backupDirectory) const {
    const auto backup = registryOptimizer_.backupRunKeys(backupDirectory);
    if (!backup.has_value()) {
        return false;
    }

    if (item.location.find("Registry") != std::string::npos) {
        return registryOptimizer_.deleteRunValue(kRunSubKey, toWide(item.name), item.location.find("Current User") != std::string::npos);
    }

    std::error_code error;
    const std::filesystem::path target(item.command);
    const auto disabledPath = target.parent_path() / (target.filename().string() + ".disabled");
    std::filesystem::rename(target, disabledPath, error);
    return !error;
}

bool StartupOptimizer::delayStartupItem(const StartupItem &item, int delaySeconds) const {
    if (item.command.empty() || delaySeconds < 1) {
        return false;
    }

    const std::wstring taskName = L"PulseBoost-" + toWide(item.name);
    const int minutes = delaySeconds / 60;
    std::wstring delay = L"0000:30";
    if (minutes > 0) {
        wchar_t buffer[16] {};
        swprintf_s(buffer, L"000%d:00", minutes);
        delay = buffer;
    }

    const std::wstring command = L"schtasks.exe /Create /F /SC ONLOGON /DELAY " + delay + L" /TN \"" + taskName +
                                 L"\" /TR \"" + toWide(item.command) + L"\"";
    DWORD exitCode = 1;
    return runProcessHidden(command, &exitCode) && exitCode == 0;
}

}  // namespace pulseboost
