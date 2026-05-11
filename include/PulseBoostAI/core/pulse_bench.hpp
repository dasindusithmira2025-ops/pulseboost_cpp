#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pulseboost {

struct BenchmarkResult {
    double cpuScore = 0.0;
    double ramBandwidthMBps = 0.0;
    double diskReadMBps = 0.0;
    double networkLatencyMs = 0.0;
    double gpuScore = 0.0;
    double compositeScore = 0.0;
    double pulseScore = 0.0;
    std::string grade = "F";
    std::int64_t timestamp = 0;
};

struct BenchmarkDelta {
    BenchmarkResult before;
    BenchmarkResult after;
    double percentChange = 0.0;
    double scoreDelta = 0.0;
};

class PulseBench {
public:
    BenchmarkResult runQuick();
    BenchmarkResult runFull();
    std::vector<BenchmarkResult> loadHistory() const;
    std::optional<BenchmarkDelta> latestDelta() const;

private:
    BenchmarkResult run(bool fullRun);
    void saveResult(const BenchmarkResult &result) const;
    double computeComposite(const BenchmarkResult &result) const;
    std::string computeGrade(double compositeScore) const;
};

}  // namespace pulseboost
