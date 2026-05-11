#include "PulseBoostAI/core/pulse_bench.hpp"

#include <Windows.h>

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "PulseBoostAI/common/windows_utils.hpp"
#include "PulseBoostAI/modules/network_optimizer.hpp"

namespace pulseboost {

namespace {

std::filesystem::path benchmarkHistoryPath() {
    std::filesystem::path path = std::filesystem::path("data") / "pulsebench_history.jsonl";
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    return path;
}

std::int64_t nowEpochMs() {
    return QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
}

BenchmarkResult fromJsonObject(const QJsonObject &object) {
    BenchmarkResult result;
    result.cpuScore = object.value("cpuScore").toDouble();
    result.ramBandwidthMBps = object.value("ramBandwidthMBps").toDouble();
    result.diskReadMBps = object.value("diskReadMBps").toDouble();
    result.networkLatencyMs = object.value("networkLatencyMs").toDouble();
    result.gpuScore = object.value("gpuScore").toDouble();
    result.compositeScore = object.value("compositeScore").toDouble();
    result.pulseScore = object.value("pulseScore").toDouble();
    result.grade = object.value("grade").toString().toStdString();
    result.timestamp = object.value("timestamp").toVariant().toLongLong();
    return result;
}

QJsonObject toJsonObject(const BenchmarkResult &result) {
    QJsonObject object;
    object["cpuScore"] = result.cpuScore;
    object["ramBandwidthMBps"] = result.ramBandwidthMBps;
    object["diskReadMBps"] = result.diskReadMBps;
    object["networkLatencyMs"] = result.networkLatencyMs;
    object["gpuScore"] = result.gpuScore;
    object["compositeScore"] = result.compositeScore;
    object["pulseScore"] = result.pulseScore;
    object["grade"] = QString::fromStdString(result.grade);
    object["timestamp"] = static_cast<qint64>(result.timestamp);
    return object;
}

double runCpuBenchmark(std::chrono::milliseconds duration) {
    volatile std::uint64_t accumulator = 0x123456789ABCDEF0ull;
    const auto start = std::chrono::steady_clock::now();
    std::uint64_t iterations = 0;
    while (std::chrono::steady_clock::now() - start < duration) {
        accumulator ^= (accumulator << 13);
        accumulator ^= (accumulator >> 7);
        accumulator ^= (accumulator << 17);
        accumulator += 0x9E3779B97F4A7C15ull;
        ++iterations;
    }
    const double seconds = std::max(0.001, std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
    return (static_cast<double>(iterations) / seconds) / 1000000.0;
}

double runMemoryBenchmark(std::size_t bytes, int passes) {
    std::vector<std::uint8_t> source(bytes, 0x5A);
    std::vector<std::uint8_t> destination(bytes, 0x00);
    const auto start = std::chrono::steady_clock::now();
    for (int pass = 0; pass < passes; ++pass) {
        std::memcpy(destination.data(), source.data(), bytes);
        std::swap(source, destination);
    }
    const double seconds = std::max(0.001, std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
    const double totalMb = (static_cast<double>(bytes) * static_cast<double>(passes)) / (1024.0 * 1024.0);
    return totalMb / seconds;
}

double runDiskBenchmark(std::size_t bytes) {
    const std::filesystem::path filePath = std::filesystem::temp_directory_path() / "pulseboost_bench.tmp";
    {
        std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
        if (!output.is_open()) {
            return 0.0;
        }
        std::vector<char> block(1024 * 1024, 'P');
        std::size_t remaining = bytes;
        while (remaining > 0) {
            const std::size_t chunk = std::min(remaining, block.size());
            output.write(block.data(), static_cast<std::streamsize>(chunk));
            remaining -= chunk;
        }
    }

    HANDLE fileHandle = CreateFileW(filePath.wstring().c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    nullptr,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                                    nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        std::error_code error;
        std::filesystem::remove(filePath, error);
        return 0.0;
    }

    std::vector<char> buffer(1024 * 1024);
    DWORD bytesRead = 0;
    std::size_t totalRead = 0;
    const auto start = std::chrono::steady_clock::now();
    while (ReadFile(fileHandle, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr) && bytesRead > 0) {
        totalRead += bytesRead;
    }
    CloseHandle(fileHandle);

    std::error_code error;
    std::filesystem::remove(filePath, error);
    const double seconds = std::max(0.001, std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
    return (static_cast<double>(totalRead) / (1024.0 * 1024.0)) / seconds;
}

double runLatencyBenchmark() {
    NetworkOptimizer optimizer;
    const int measured = optimizer.measureLatency();
    return measured >= 0 ? static_cast<double>(measured) : 100.0;
}

}  // namespace

BenchmarkResult PulseBench::runQuick() {
    return run(false);
}

BenchmarkResult PulseBench::runFull() {
    return run(true);
}

BenchmarkResult PulseBench::run(bool fullRun) {
    BenchmarkResult result;
    result.cpuScore = runCpuBenchmark(fullRun ? std::chrono::milliseconds(4000) : std::chrono::milliseconds(1800));
    result.ramBandwidthMBps = runMemoryBenchmark(fullRun ? 256ull * 1024ull * 1024ull : 128ull * 1024ull * 1024ull,
                                                 fullRun ? 6 : 4);
    result.diskReadMBps = runDiskBenchmark(fullRun ? 128ull * 1024ull * 1024ull : 64ull * 1024ull * 1024ull);
    result.networkLatencyMs = runLatencyBenchmark();
    result.gpuScore = 0.0;
    result.compositeScore = computeComposite(result);
    result.pulseScore = std::clamp(result.compositeScore * 6.5, 0.0, 1000.0);
    result.grade = computeGrade(result.compositeScore);
    result.timestamp = nowEpochMs();
    saveResult(result);
    return result;
}

void PulseBench::saveResult(const BenchmarkResult &result) const {
    std::ofstream output(benchmarkHistoryPath(), std::ios::app);
    if (!output.is_open()) {
        return;
    }
    output << QJsonDocument(toJsonObject(result)).toJson(QJsonDocument::Compact).toStdString() << '\n';
}

std::vector<BenchmarkResult> PulseBench::loadHistory() const {
    std::vector<BenchmarkResult> results;
    std::ifstream input(benchmarkHistoryPath());
    if (!input.is_open()) {
        return results;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        QJsonParseError error {};
        const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(line), &error);
        if (error.error == QJsonParseError::NoError && document.isObject()) {
            results.push_back(fromJsonObject(document.object()));
        }
    }
    return results;
}

std::optional<BenchmarkDelta> PulseBench::latestDelta() const {
    auto history = loadHistory();
    if (history.size() < 2) {
        return std::nullopt;
    }
    const BenchmarkResult &before = history[history.size() - 2];
    const BenchmarkResult &after = history[history.size() - 1];
    BenchmarkDelta delta;
    delta.before = before;
    delta.after = after;
    delta.scoreDelta = after.compositeScore - before.compositeScore;
    delta.percentChange = before.compositeScore > 0.0 ? (delta.scoreDelta / before.compositeScore) * 100.0 : 0.0;
    return delta;
}

double PulseBench::computeComposite(const BenchmarkResult &result) const {
    const double cpuComponent = result.cpuScore * 18.0;
    const double ramComponent = result.ramBandwidthMBps * 0.9;
    const double diskComponent = result.diskReadMBps * 0.12;
    const double networkComponent = std::max(0.0, 120.0 - result.networkLatencyMs) * 3.0;
    return cpuComponent + ramComponent + diskComponent + networkComponent;
}

std::string PulseBench::computeGrade(double compositeScore) const {
    if (compositeScore >= 1500.0) {
        return "S";
    }
    if (compositeScore >= 1100.0) {
        return "A";
    }
    if (compositeScore >= 800.0) {
        return "B";
    }
    if (compositeScore >= 550.0) {
        return "C";
    }
    if (compositeScore >= 350.0) {
        return "D";
    }
    return "F";
}

}  // namespace pulseboost

