#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace pulseboost {

struct FileEntry {
    std::string path;
    std::uint64_t bytes = 0;
};

struct DuplicateGroup {
    std::vector<FileEntry> files;
    std::uint64_t reclaimableBytes = 0;
};

struct ProcessInfo {
    std::uint32_t pid = 0;
    std::string name;
    std::string imagePath;
    double cpuPercent = 0.0;
    double memoryMb = 0.0;
    bool isCritical = false;
};

struct StartupItem {
    std::string name;
    std::string command;
    std::string location;
    bool enabled = true;
    int impactScore = 0;
};

struct ServiceInfo {
    std::string name;
    std::string displayName;
    std::string state;
    std::string startMode;
    bool canStop = false;
};

struct DriverInfo {
    std::string deviceName;
    std::string driverVersion;
    std::string provider;
};

struct StorageCategory {
    std::string name;
    std::uint64_t bytes = 0;
    std::string accentColor = "#58a6ff";
};

struct MemorySummary {
    double usedMb = 0.0;
    double totalMb = 0.0;
    double usedPercent = 0.0;
    std::vector<ProcessInfo> topConsumers;
};

struct DiskSummary {
    std::uint64_t totalBytes = 0;
    std::uint64_t usedBytes = 0;
    double usedPercent = 0.0;
    double readMbps = 0.0;
    double writeMbps = 0.0;
    std::vector<StorageCategory> categories;
    std::vector<FileEntry> largeFiles;
};

struct CleanupResult {
    std::uint64_t bytesRecovered = 0;
    int filesRemoved = 0;
    std::vector<std::string> actions;
};

struct ActionRecord {
    std::string timestampUtc;
    std::string action;
    std::string details;
    bool success = false;
};

struct OptimizationPlanStep {
    std::string title;
    std::string details;
    bool execute = false;
};

struct AgentDecision {
    std::string reasoningPrompt;
    std::vector<OptimizationPlanStep> plan;
    bool shouldExecutePlan = false;
};

struct SystemSnapshot {
    std::chrono::system_clock::time_point capturedAt = std::chrono::system_clock::now();
    double cpuUsagePercent = 0.0;
    double ramUsagePercent = 0.0;
    double ramUsedMb = 0.0;
    double ramTotalMb = 0.0;
    double diskUsagePercent = 0.0;
    double diskUsedGb = 0.0;
    double diskTotalGb = 0.0;
    double diskReadMbps = 0.0;
    double diskWriteMbps = 0.0;
    double gpuUsagePercent = 0.0;
    double networkMbps = 0.0;
    int startupPrograms = 0;
    int runningServices = 0;
    int healthScore = 100;
    std::string summary;
    std::vector<std::string> issues;
    std::vector<ProcessInfo> heavyProcesses;
    std::vector<StartupItem> startupItems;
    std::vector<ServiceInfo> services;
    std::vector<DriverInfo> drivers;
    std::vector<StorageCategory> storageCategories;
};

struct ChatResponse {
    bool actionExecuted = false;
    std::string message;
};

}  // namespace pulseboost
