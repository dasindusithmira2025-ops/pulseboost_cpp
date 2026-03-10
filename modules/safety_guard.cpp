#include "PulseBoostAI/modules/safety_guard.hpp"

#include <Windows.h>

// RESTOREPOINTINFOW / SRSetRestorePointW structures (from srrestoreptapi.h)
#ifndef BEGIN_SYSTEM_CHANGE
#  define BEGIN_SYSTEM_CHANGE   100
#  define APPLICATION_INSTALL   0
typedef struct _RESTOREPTINFOW {
    DWORD  dwEventType;
    DWORD  dwRestorePtType;
    INT64  llSequenceNumber;
    WCHAR  szDescription[256];
} RESTOREPOINTINFOW, *LPRESTOREPOINTINFOW;
typedef struct _SMGRSTATUS {
    DWORD nStatus;
    INT64 llSequenceNumber;
} STATEMGRSTATUS, *LPSTATEMGRSTATUS;
#endif

#include <fstream>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

bool SafetyGuard::createRestorePoint(const std::wstring &description) {
    // Dynamically load SRSetRestorePointW from srclient.dll (ships with Windows)
    HMODULE hSrClient = LoadLibraryW(L"srclient.dll");
    if (!hSrClient) {
        logAction("safety-guard", "srclient.dll not available", false);
        return false;
    }

    using SRSetRestorePointWFn = BOOL (WINAPI *)(LPRESTOREPOINTINFOW, LPSTATEMGRSTATUS);
    auto fnSrSet = reinterpret_cast<SRSetRestorePointWFn>(
        GetProcAddress(hSrClient, "SRSetRestorePointW"));

    if (!fnSrSet) {
        FreeLibrary(hSrClient);
        logAction("safety-guard", "SRSetRestorePointW not found in srclient.dll", false);
        return false;
    }

    RESTOREPOINTINFOW rpInfo {};
    rpInfo.dwEventType      = BEGIN_SYSTEM_CHANGE;
    rpInfo.dwRestorePtType  = APPLICATION_INSTALL;
    rpInfo.llSequenceNumber = 0;
    const std::wstring desc = description.substr(0, 255);
    wcsncpy_s(rpInfo.szDescription, desc.c_str(), _TRUNCATE);

    STATEMGRSTATUS smStatus {};
    const BOOL ok = fnSrSet(&rpInfo, &smStatus);
    FreeLibrary(hSrClient);

    logAction("safety-guard",
              ok ? "Restore point created: " +
                       std::string(description.begin(), description.end())
                 : "Restore point failed, nStatus=" +
                       std::to_string(smStatus.nStatus),
              ok != FALSE);
    return ok != FALSE;
}

bool SafetyGuard::backupRegistryKey(HKEY root, const std::wstring &subKey, const std::wstring &dstPath) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, subKey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }
    const bool ok = (RegSaveKeyW(key, dstPath.c_str(), nullptr) == ERROR_SUCCESS);
    RegCloseKey(key);
    logAction("reg-backup", std::string(dstPath.begin(), dstPath.end()), ok);
    return ok;
}

void SafetyGuard::logAction(const std::string &module, const std::string &detail, bool success) {
    std::ofstream out("logs/safety.log", std::ios::app);
    out << currentTimestampUtc() << ',' << module << ',' << (success ? "OK" : "FAIL")
        << ',' << detail << '\n';
}

}  // namespace pulseboost
