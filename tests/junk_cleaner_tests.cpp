#include "PulseBoostAI/modules/junk_cleaner.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

int failures = 0;

void expect(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

std::filesystem::path makeTempRoot(const char *name) {
    auto root = std::filesystem::temp_directory_path() / name;
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root, error);
    return root;
}

void writeFile(const std::filesystem::path &path, const std::string &content) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << content;
}

}  // namespace

int runJunkCleanerTests() {
    {
        const auto target = makeTempRoot("pulseboost_junk_dry_run");
        writeFile(target / "junk.tmp", "123456789");

        pulseboost::JunkCleaner cleaner;
        pulseboost::CleanupOptions options;
        options.dryRun = true;
        options.overrideTargets = {target};

        const auto result = cleaner.cleanSafeTargets(options);
        expect(result.dryRun, "dry-run flag should be reflected in result");
        expect(result.filesScanned == 1, "dry-run should scan one file");
        expect(result.bytesRecovered == 9, "dry-run should estimate file bytes");
        expect(std::filesystem::exists(target / "junk.tmp"), "dry-run must not remove files");
        std::error_code error;
        std::filesystem::remove_all(target, error);
    }

    {
        const auto target = makeTempRoot("pulseboost_junk_quarantine");
        const auto quarantine = makeTempRoot("pulseboost_junk_quarantine_store");
        writeFile(target / "junk.tmp", "abcdef");

        pulseboost::JunkCleaner cleaner;
        pulseboost::CleanupOptions options;
        options.mode = pulseboost::CleanupMode::Quarantine;
        options.quarantineRoot = quarantine;
        options.overrideTargets = {target};

        const auto result = cleaner.cleanSafeTargets(options);
        expect(!result.permanentDelete, "quarantine cleanup must not be permanent delete");
        expect(result.filesQuarantined == 1, "quarantine cleanup should move one file");
        expect(!std::filesystem::exists(target / "junk.tmp"), "quarantined file should leave target");
        expect(std::filesystem::exists(quarantine / "junk.tmp"), "quarantined file should exist in quarantine");
        std::error_code error;
        std::filesystem::remove_all(target, error);
        std::filesystem::remove_all(quarantine, error);
    }

    return failures;
}
