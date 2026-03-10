#include "PulseBoostAI/data/optimization_history.hpp"

#include <fstream>
#include <sstream>

namespace pulseboost {

OptimizationHistory::OptimizationHistory(std::filesystem::path outputDirectory) {
    std::error_code error;
    std::filesystem::create_directories(outputDirectory, error);
    historyFile_ = outputDirectory / "optimization_history.csv";
    if (!std::filesystem::exists(historyFile_)) {
        std::ofstream output(historyFile_);
        output << "timestamp,action,details,success\n";
    }
}

void OptimizationHistory::record(const ActionRecord &record) {
    std::ofstream output(historyFile_, std::ios::app);
    output << record.timestampUtc << ',' << record.action << ',' << record.details << ','
           << (record.success ? "true" : "false") << '\n';
}

std::vector<ActionRecord> OptimizationHistory::load() const {
    std::vector<ActionRecord> records;
    std::ifstream input(historyFile_);
    std::string line;
    std::getline(input, line);
    while (std::getline(input, line)) {
        std::stringstream stream(line);
        std::string token;
        ActionRecord record;
        std::getline(stream, record.timestampUtc, ',');
        std::getline(stream, record.action, ',');
        std::getline(stream, record.details, ',');
        std::getline(stream, token, ',');
        record.success = token == "true";
        records.push_back(std::move(record));
    }
    return records;
}

}  // namespace pulseboost
