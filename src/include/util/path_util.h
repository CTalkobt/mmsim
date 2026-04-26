#ifndef MMEMU_PATH_UTIL_H
#define MMEMU_PATH_UTIL_H

#include <string>
#include <vector>
#include <filesystem>

class PathUtil {
public:
    static std::string findResource(const std::string& relPath);
    static std::vector<std::string> getPluginSearchPaths();
    static std::vector<std::string> getDataSearchPaths();
};

#endif
