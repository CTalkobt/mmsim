#include "rom_importer.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

ImportResult importRoms(const RomSource& src, const std::string& machineId, const std::string& destDir, bool overwrite) {
    auto specs = romFilesFor(machineId);
    if (specs.empty()) {
        return { false, {}, "Unknown machine ID: " + machineId };
    }

    ImportResult result;
    result.success = true;

    try {
        fs::create_directories(destDir);
    } catch (const std::exception& e) {
        return { false, {}, "Failed to create destination directory: " + std::string(e.what()) };
    }

    std::vector<fs::path> copied;

    for (const auto& spec : specs) {
        fs::path srcPath = fs::path(src.basePath) / spec.srcRelPath;
        fs::path destPath = fs::path(destDir) / spec.destName;

        if (fs::exists(destPath) && !overwrite) {
            result.success = false;
            result.errorMessage = "File already exists: " + spec.destName + ". Use overwrite to replace.";
            break;
        }

        try {
            fs::copy_file(srcPath, destPath, overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none);
            copied.push_back(destPath);  // track for rollback before validating

            // Validate size
            if (fs::file_size(destPath) != spec.expectedSize) {
                result.success = false;
                result.errorMessage = "Size mismatch for " + spec.destName;
                break;
            }

            result.copiedFiles.push_back(spec.destName);
        } catch (const std::exception& e) {
            result.success = false;
            result.errorMessage = "IO Error copying " + spec.destName + ": " + e.what();
            break;
        }
    }

    if (!result.success) {
        // Rollback
        for (const auto& p : copied) {
            fs::remove(p);
        }
        result.copiedFiles.clear();
    }

    return result;
}
