#include "PulseBoostAI/core/registry_optimizer.hpp"

#include <Windows.h>

#include <filesystem>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

bool RegistryOptimizer::createRestorePoint(const std::string &description) const {
    std::wstring sanitized = toWide(description);
    for (wchar_t &ch : sanitized) {
        if (ch == L'\'' || ch == L'\r' || ch == L'\n' || ch == L'\t') {
            ch = L' ';
        }
    }

    const std::wstring command = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"Checkpoint-Computer "
                                 L"-Description '" +
                                 sanitized + L"' -RestorePointType 'MODIFY_SETTINGS'\"";
    DWORD exitCode = 1;
    return runProcessHidden(command, &exitCode) && exitCode == 0;
}

std::optional<std::filesystem::path> RegistryOptimizer::backupRunKeys(const std::filesystem::path &outputDirectory) const {
    std::error_code error;
    std::filesystem::create_directories(outputDirectory, error);
    if (error) {
        return std::nullopt;
    }

    const auto timestampDir = outputDirectory / ("run-keys-" + currentTimestampUtc());
    std::filesystem::create_directories(timestampDir, error);
    if (error) {
        return std::nullopt;
    }

    const std::wstring userCommand = L"reg.exe export \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\" \"" +
                                     timestampDir.wstring() + L"\\hkcu_run.reg\" /y";
    const std::wstring machineCommand =
        L"reg.exe export \"HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\" \"" + timestampDir.wstring() +
        L"\\hklm_run.reg\" /y";

    DWORD userExitCode = 1;
    DWORD machineExitCode = 1;
    const bool userOk = runProcessHidden(userCommand, &userExitCode) && userExitCode == 0;
    const bool machineOk = runProcessHidden(machineCommand, &machineExitCode) && machineExitCode == 0;
    if (!userOk && !machineOk) {
        return std::nullopt;
    }

    return timestampDir;
}

bool RegistryOptimizer::deleteRunValue(const std::wstring &subKey, const std::wstring &valueName, bool currentUser) const {
    HKEY rootKey = currentUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    HKEY keyHandle = nullptr;
    if (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_SET_VALUE, &keyHandle) != ERROR_SUCCESS) {
        return false;
    }

    const bool success = RegDeleteValueW(keyHandle, valueName.c_str()) == ERROR_SUCCESS;
    RegCloseKey(keyHandle);
    return success;
}

bool RegistryOptimizer::setRunValue(const std::wstring &subKey,
                                    const std::wstring &valueName,
                                    const std::wstring &valueData,
                                    bool currentUser) const {
    HKEY rootKey = currentUser ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    HKEY keyHandle = nullptr;
    if (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_SET_VALUE, &keyHandle) != ERROR_SUCCESS) {
        return false;
    }

    const DWORD bytes = static_cast<DWORD>((valueData.size() + 1) * sizeof(wchar_t));
    const bool success = RegSetValueExW(keyHandle,
                                        valueName.c_str(),
                                        0,
                                        REG_SZ,
                                        reinterpret_cast<const BYTE *>(valueData.c_str()),
                                        bytes) == ERROR_SUCCESS;
    RegCloseKey(keyHandle);
    return success;
}

}  // namespace pulseboost
