#pragma once
#include <string>
#include <vector>

namespace mega65_importer {

struct ImportResult {
    bool success;
    std::string message;
    std::vector<std::string> copiedFiles;
};

ImportResult importRoms(const std::string& machineId,
                        const std::string& sourcePath,
                        const std::string& destPath,
                        bool overwrite);
} // namespace mega65_importer
