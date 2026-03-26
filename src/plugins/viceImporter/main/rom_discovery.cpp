#include "rom_discovery.h"
#include <filesystem>

namespace fs = std::filesystem;

std::vector<RomFileSpec> romFilesFor(const std::string& machineId) {
    if (machineId == "vic20") {
        return {
            { "VIC20/basic",    "basic.bin",   8192 },
            { "VIC20/kernal",   "kernal.bin",  8192 },
            { "VIC20/chargen",  "char.bin",    4096 }
        };
    }
    return {};
}

std::vector<RomSource> discoverSourcesInPaths(const std::string& machineId,
                                               const std::vector<std::string>& searchPaths) {
    std::vector<RomSource> found;
    auto specs = romFilesFor(machineId);
    if (specs.empty()) return found;

    for (const auto& path : searchPaths) {
        if (!fs::exists(path)) continue;

        bool allPresent = true;
        for (const auto& spec : specs) {
            fs::path fullPath = fs::path(path) / spec.srcRelPath;
            if (!fs::exists(fullPath) || fs::file_size(fullPath) != spec.expectedSize) {
                allPresent = false;
                break;
            }
        }

        if (allPresent) {
            std::string label = (path == "rom") ? "Bundled ROMs (rom/)" : "VICE at " + path;
            found.push_back({ label, path });
        }
    }

    return found;
}

std::vector<RomSource> discoverSources(const std::string& machineId) {
    std::vector<std::string> candidates = {
        "rom",
        "/usr/share/vice",
        "/usr/local/share/vice",
        "/usr/share/games/vice",
        "/opt/vice",
        "/opt/vice/lib/vice",
        "/usr/lib/vice"
    };

    const char* home = std::getenv("HOME");
    if (home) {
        std::string h(home);
        candidates.push_back(h + "/.local/share/vice");
        candidates.push_back(h + "/Applications/VICE.app/Contents/Resources");
    }

#ifdef _WIN32
    candidates.push_back("C:\\Program Files\\VICE");
    candidates.push_back("C:\\Program Files (x86)\\VICE");
#endif

    return discoverSourcesInPaths(machineId, candidates);
}
