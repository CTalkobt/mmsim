#include "kickassembler.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <filesystem>

namespace fs = std::filesystem;

bool KickAssemblerBackend::isaSupported(const std::string& isa) const {
    return isa == "6502" || isa == "65c02" || isa == "45gs02" || isa == "65ce02";
}

AssemblerResult KickAssemblerBackend::assemble(const std::string& sourcePath, const std::string& outputPath) {
    AssemblerResult result;
    result.success = false;
    result.errorCount = 0;
    result.warningCount = 0;

    std::string jarPath = m_config.binaryPath;
    if (jarPath.empty()) {
        jarPath = "tools/KickAss65CE02.jar";
    }

    if (!fs::exists(jarPath)) {
        result.errorMessage = "KickAssembler JAR not found at: " + jarPath;
        return result;
    }

    std::stringstream ss;
    ss << "java -jar " << jarPath << " ";
    ss << sourcePath << " ";
    ss << "-o " << outputPath << " ";
    ss << "-v "; // verbose for better parsing if we implemented it
    ss << m_config.extraOptions;

    std::string cmd = ss.str();
    int ret = std::system(cmd.c_str());

    if (ret == 0) {
        result.success = true;
        result.outputPath = outputPath;
        
        // KickAss generates .sym and .lst by default in the same dir as output
        fs::path p(outputPath);
        result.symPath = p.replace_extension(".sym").string();
        result.listPath = p.replace_extension(".lst").string();
    } else {
        result.errorMessage = "KickAssembler failed with exit code " + std::to_string(ret);
    }

    return result;
}
