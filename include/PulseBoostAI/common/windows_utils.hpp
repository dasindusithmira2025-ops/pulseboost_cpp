#pragma once

#include <Windows.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <cwchar>

namespace pulseboost {

inline std::wstring toWide(const std::string &value) {
    if (value.empty()) {
        return {};
    }

    const int required = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    std::wstring result(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), required);
    result.resize(static_cast<std::size_t>(required - 1));
    return result;
}

inline std::string fromWide(const std::wstring &value) {
    if (value.empty()) {
        return {};
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), required, nullptr, nullptr);
    result.resize(static_cast<std::size_t>(required - 1));
    return result;
}

inline std::string formatBytes(std::uint64_t bytes) {
    constexpr const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    std::size_t unitIndex = 0;
    while (value >= 1024.0 && unitIndex < std::size(units) - 1) {
        value /= 1024.0;
        ++unitIndex;
    }

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(unitIndex == 0 ? 0 : 2) << value << ' ' << units[unitIndex];
    return stream.str();
}

inline std::optional<std::filesystem::path> expandedPath(const std::wstring &path) {
    std::wstring buffer(MAX_PATH, L'\0');
    const DWORD written = ExpandEnvironmentStringsW(path.c_str(), buffer.data(), static_cast<DWORD>(buffer.size()));
    if (written == 0) {
        return std::nullopt;
    }

    if (written > buffer.size()) {
        buffer.resize(written, L'\0');
        ExpandEnvironmentStringsW(path.c_str(), buffer.data(), static_cast<DWORD>(buffer.size()));
    }

    buffer.resize(wcslen(buffer.c_str()));
    return std::filesystem::path(buffer);
}

inline std::string currentTimestampUtc() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t timeValue = std::chrono::system_clock::to_time_t(now);
    std::tm utcTime {};
    gmtime_s(&utcTime, &timeValue);

    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

inline std::uint64_t fileTimeToUint64(const FILETIME &value) {
    ULARGE_INTEGER integerValue {};
    integerValue.LowPart = value.dwLowDateTime;
    integerValue.HighPart = value.dwHighDateTime;
    return integerValue.QuadPart;
}

inline bool runProcessHidden(const std::wstring &commandLine, DWORD *exitCode = nullptr, DWORD timeoutMs = 60000) {
    STARTUPINFOW startupInfo {};
    PROCESS_INFORMATION processInfo {};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_HIDE;

    std::wstring commandBuffer = commandLine;
    if (!CreateProcessW(nullptr,
                        commandBuffer.data(),
                        nullptr,
                        nullptr,
                        FALSE,
                        CREATE_NO_WINDOW,
                        nullptr,
                        nullptr,
                        &startupInfo,
                        &processInfo)) {
        return false;
    }

    const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, timeoutMs);
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(processInfo.hProcess, 1);
        if (exitCode != nullptr) {
            *exitCode = WAIT_TIMEOUT;
        }
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        return false;
    }

    if (exitCode != nullptr) {
        GetExitCodeProcess(processInfo.hProcess, exitCode);
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return true;
}

}  // namespace pulseboost

