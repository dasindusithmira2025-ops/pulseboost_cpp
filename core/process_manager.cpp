#include "PulseBoostAI/core/process_manager.hpp"

#include <TlHelp32.h>
#include <Psapi.h>

#include <algorithm>
#include <string>
#include <unordered_set>

#include "PulseBoostAI/common/windows_utils.hpp"

namespace pulseboost {

namespace {

std::string queryImagePath(HANDLE processHandle) {
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD size = static_cast<DWORD>(buffer.size());
    if (!QueryFullProcessImageNameW(processHandle, 0, buffer.data(), &size)) {
        return {};
    }

    buffer.resize(size);
    return fromWide(buffer);
}

bool processCriticalHeuristic(const PROCESSENTRY32W &entry, const std::string &imagePath) {
    const std::wstring lowerName = entry.szExeFile;
    if (_wcsicmp(lowerName.c_str(), L"System") == 0 || _wcsicmp(lowerName.c_str(), L"Registry") == 0) {
        return true;
    }

    return imagePath.find("\\Windows\\System32\\") != std::string::npos;
}

}  // namespace

std::vector<ProcessInfo> ProcessManager::enumerateProcesses() {
    std::vector<ProcessInfo> processes;
    std::unordered_set<std::uint32_t> activePids;

    HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshotHandle == INVALID_HANDLE_VALUE) {
        return processes;
    }

    PROCESSENTRY32W entry {};
    entry.dwSize = sizeof(entry);
    if (!Process32FirstW(snapshotHandle, &entry)) {
        CloseHandle(snapshotHandle);
        return processes;
    }

    FILETIME systemTime {};
    GetSystemTimeAsFileTime(&systemTime);
    const std::uint64_t currentWallClock = fileTimeToUint64(systemTime);

    do {
        activePids.insert(entry.th32ProcessID);

        HANDLE processHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, entry.th32ProcessID);
        if (processHandle == nullptr) {
            continue;
        }

        PROCESS_MEMORY_COUNTERS_EX counters {};
        GetProcessMemoryInfo(processHandle, reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters), sizeof(counters));

        FILETIME creationTime {}, exitTime {}, kernelTime {}, userTime {};
        std::uint64_t processTime = 0;
        if (GetProcessTimes(processHandle, &creationTime, &exitTime, &kernelTime, &userTime)) {
            processTime = fileTimeToUint64(kernelTime) + fileTimeToUint64(userTime);
        }

        double cpuPercent = 0.0;
        {
            std::lock_guard<std::mutex> lock(cpuSamplesMutex_);
            const auto iterator = cpuSamples_.find(entry.th32ProcessID);
            if (iterator != cpuSamples_.end()) {
                const std::uint64_t processDelta = processTime - iterator->second.processTime;
                const std::uint64_t wallDelta = currentWallClock - iterator->second.wallClock;
                if (wallDelta > 0) {
                    cpuPercent = (static_cast<double>(processDelta) / static_cast<double>(wallDelta)) * 100.0;
                }
            }
            cpuSamples_[entry.th32ProcessID] = CpuSample {.processTime = processTime, .wallClock = currentWallClock};
        }

        const std::string imagePath = queryImagePath(processHandle);
        ProcessInfo info;
        info.pid = entry.th32ProcessID;
        info.name = fromWide(entry.szExeFile);
        info.imagePath = imagePath;
        info.cpuPercent = std::clamp(cpuPercent, 0.0, 100.0);
        info.memoryMb = static_cast<double>(counters.WorkingSetSize) / (1024.0 * 1024.0);
        info.isCritical = processCriticalHeuristic(entry, imagePath);
        processes.push_back(std::move(info));
        CloseHandle(processHandle);
    } while (Process32NextW(snapshotHandle, &entry));

    CloseHandle(snapshotHandle);

    // Drop samples for exited processes to keep map bounded.
    {
        std::lock_guard<std::mutex> lock(cpuSamplesMutex_);
        for (auto it = cpuSamples_.begin(); it != cpuSamples_.end();) {
            if (!activePids.contains(it->first)) {
                it = cpuSamples_.erase(it);
            } else {
                ++it;
            }
        }
    }

    std::sort(processes.begin(), processes.end(), [](const ProcessInfo &left, const ProcessInfo &right) {
        if (left.cpuPercent == right.cpuPercent) {
            return left.memoryMb > right.memoryMb;
        }
        return left.cpuPercent > right.cpuPercent;
    });

    return processes;
}

bool ProcessManager::setPriority(std::uint32_t pid, DWORD priorityClass) const {
    HANDLE processHandle = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (processHandle == nullptr) {
        return false;
    }

    const bool success = SetPriorityClass(processHandle, priorityClass) == TRUE;
    CloseHandle(processHandle);
    return success;
}

bool ProcessManager::setAffinity(std::uint32_t pid, std::uint64_t affinityMask) const {
    HANDLE processHandle = OpenProcess(PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (processHandle == nullptr) {
        return false;
    }

    const bool success = SetProcessAffinityMask(processHandle, static_cast<DWORD_PTR>(affinityMask)) == TRUE;
    CloseHandle(processHandle);
    return success;
}

bool ProcessManager::suspendProcess(std::uint32_t pid) const {
    HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshotHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool suspendedAny = false;
    THREADENTRY32 threadEntry {};
    threadEntry.dwSize = sizeof(threadEntry);
    if (Thread32First(snapshotHandle, &threadEntry)) {
        do {
            if (threadEntry.th32OwnerProcessID != pid) {
                continue;
            }

            HANDLE threadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
            if (threadHandle != nullptr) {
                SuspendThread(threadHandle);
                CloseHandle(threadHandle);
                suspendedAny = true;
            }
        } while (Thread32Next(snapshotHandle, &threadEntry));
    }

    CloseHandle(snapshotHandle);
    return suspendedAny;
}

bool ProcessManager::resumeProcess(std::uint32_t pid) const {
    HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshotHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool resumedAny = false;
    THREADENTRY32 threadEntry {};
    threadEntry.dwSize = sizeof(threadEntry);
    if (Thread32First(snapshotHandle, &threadEntry)) {
        do {
            if (threadEntry.th32OwnerProcessID != pid) {
                continue;
            }

            HANDLE threadHandle = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
            if (threadHandle != nullptr) {
                while (ResumeThread(threadHandle) > 0) {
                }
                CloseHandle(threadHandle);
                resumedAny = true;
            }
        } while (Thread32Next(snapshotHandle, &threadEntry));
    }

    CloseHandle(snapshotHandle);
    return resumedAny;
}

}  // namespace pulseboost
