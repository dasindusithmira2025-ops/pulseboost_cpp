#include "PulseBoostAI/core/registry_optimizer.hpp"

#include <Windows.h>

#include <filesystem>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

bool RegistryOptimizer::createRestorePoint(const std::string &description) const {
    const std::wstring command = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"Checkpoint-Computer "
                                 L"-Description '" +
                                 toWide(description) + L"' -RestorePointType 'MODIFY_SETTINGS'\"";
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

}  // namespace pulseboost
