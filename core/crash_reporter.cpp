#include "PulseBoostAI/core/crash_reporter.hpp"

#include <Windows.h>
#include <DbgHelp.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "Dbghelp.lib")

namespace pulseboost {

namespace {
std::string gDumpDirectory;
std::string gPendingCrashArtifact;

std::filesystem::path latestCrashArtifact(const std::filesystem::path &directory) {
    std::error_code error;
    if (!std::filesystem::exists(directory, error) || error) {
        return {};
    }

    std::filesystem::path latest;
    auto latestTime = std::filesystem::file_time_type::min();
    for (const auto &entry : std::filesystem::directory_iterator(directory, error)) {
        if (error || !entry.is_regular_file()) {
            continue;
        }

        const auto extension = entry.path().extension().string();
        if (extension != ".log" && extension != ".dmp") {
            continue;
        }

        const auto modified = entry.last_write_time(error);
        if (error) {
            continue;
        }

        if (latest.empty() || modified > latestTime) {
            latest = entry.path();
            latestTime = modified;
        }
    }

    return latest;
}

LONG WINAPI unhandledExceptionHandler(EXCEPTION_POINTERS *exceptionInfo) {
    if (gDumpDirectory.empty()) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    std::error_code directoryError;
    std::filesystem::create_directories(gDumpDirectory, directoryError);

    SYSTEMTIME time;
    GetLocalTime(&time);

    std::ostringstream stamp;
    stamp << time.wYear << '-' << std::setfill('0') << std::setw(2) << time.wMonth << '-'
          << std::setw(2) << time.wDay << '_' << std::setw(2) << time.wHour << '-'
          << std::setw(2) << time.wMinute << '-' << std::setw(2) << time.wSecond;

    const std::filesystem::path base = std::filesystem::path(gDumpDirectory) / ("PulseBoost_Crash_" + stamp.str());
    const std::filesystem::path dumpPath = base.string() + ".dmp";
    const std::filesystem::path logPath = base.string() + ".log";

    HANDLE file = CreateFileW(dumpPath.wstring().c_str(),
                              GENERIC_WRITE,
                              0,
                              nullptr,
                              CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL,
                              nullptr);

    if (file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ExceptionPointers = exceptionInfo;
        dumpInfo.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(),
                          GetCurrentProcessId(),
                          file,
                          MiniDumpNormal,
                          &dumpInfo,
                          nullptr,
                          nullptr);
        CloseHandle(file);
    }

    std::ofstream logFile(logPath, std::ios::out | std::ios::trunc);
    logFile << "timestamp=" << stamp.str() << '\n';
    logFile << "exception_code=0x" << std::hex
            << (exceptionInfo && exceptionInfo->ExceptionRecord ? exceptionInfo->ExceptionRecord->ExceptionCode : 0)
            << std::dec << '\n';
    const auto address = exceptionInfo && exceptionInfo->ExceptionRecord
                             ? reinterpret_cast<std::uintptr_t>(exceptionInfo->ExceptionRecord->ExceptionAddress)
                             : 0;
    logFile << "exception_address=0x" << std::hex << address << std::dec << '\n';
    logFile << "process_id=" << GetCurrentProcessId() << '\n';
    logFile << "thread_id=" << GetCurrentThreadId() << '\n';
    logFile << "dump_path=" << dumpPath.string() << '\n';
    logFile.close();

    gPendingCrashArtifact = logPath.string();
    std::cerr << "CRITICAL ERROR: Application crashed. Log saved to " << logPath.string() << "\n";
    return EXCEPTION_EXECUTE_HANDLER;
}

}  // namespace

void CrashReporter::initialize(const std::string &dumpDirectory) {
    gDumpDirectory = dumpDirectory;

    std::error_code error;
    std::filesystem::create_directories(gDumpDirectory, error);
    const auto latest = latestCrashArtifact(std::filesystem::path(gDumpDirectory));
    gPendingCrashArtifact = latest.empty() ? std::string() : latest.string();

    SetUnhandledExceptionFilter(unhandledExceptionHandler);
}

void CrashReporter::triggerCrash() {
    volatile int *ptr = nullptr;
    *ptr = 42;
}

bool CrashReporter::hasPendingCrashReport() {
    if (gPendingCrashArtifact.empty()) {
        return false;
    }

    std::error_code error;
    return std::filesystem::exists(gPendingCrashArtifact, error) && !error;
}

std::string CrashReporter::pendingCrashReportPath() {
    if (!hasPendingCrashReport()) {
        return {};
    }
    return gPendingCrashArtifact;
}

bool CrashReporter::clearPendingCrashReport() {
    if (!hasPendingCrashReport()) {
        gPendingCrashArtifact.clear();
        return false;
    }

    std::error_code error;
    const std::filesystem::path selected(gPendingCrashArtifact);
    const std::filesystem::path stem = selected.parent_path() / selected.stem();

    std::vector<std::filesystem::path> candidates {
        selected,
        stem.string() + ".log",
        stem.string() + ".dmp"
    };

    bool removedAny = false;
    for (const auto &candidate : candidates) {
        if (std::filesystem::exists(candidate, error)) {
            removedAny = std::filesystem::remove(candidate, error) || removedAny;
        }
    }

    gPendingCrashArtifact.clear();
    return removedAny;
}

}  // namespace pulseboost
