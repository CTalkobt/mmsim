#include "util/path_util.h"
#include <filesystem>
#include <cstdlib>
#include <vector>

namespace fs = std::filesystem;

std::vector<std::string> PathUtil::getPluginSearchPaths() {
    std::vector<std::string> paths;
    paths.push_back("./lib"); // Local dev path

    const char* home = std::getenv("HOME");
    if (home) {
        paths.push_back(std::string(home) + "/.local/lib/mmsim/plugins");
    }

    paths.push_back("/usr/local/lib/mmsim/plugins");
    paths.push_back("/usr/lib/mmsim/plugins");

    return paths;
}

std::vector<std::string> PathUtil::getDataSearchPaths() {
    std::vector<std::string> paths;
    paths.push_back("."); // Local dev path

    const char* home = std::getenv("HOME");
    if (home) {
        paths.push_back(std::string(home) + "/.local/share/mmsim");
    }

    paths.push_back("/usr/local/share/mmsim");
    paths.push_back("/usr/share/mmsim");

    return paths;
}

std::string PathUtil::findResource(const std::string& relPath) {
    if (fs::exists(relPath)) return relPath;

    for (const auto& base : getDataSearchPaths()) {
        fs::path p = fs::path(base) / relPath;
        if (fs::exists(p)) return p.string();
    }

    return relPath; // Fallback to original
}
