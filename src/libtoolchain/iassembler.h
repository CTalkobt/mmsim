#pragma once

#include <string>
#include <vector>
#include <map>

/**
 * Result of an assembly operation.
 */
struct AssemblerResult {
    bool success;
    std::string errorMessage;
    std::string outputPath;
    std::string symPath;
    std::string listPath;
    int errorCount;
    int warningCount;
};

/**
 * Configuration/Metadata for an assembler backend.
 */
struct AssemblerConfig {
    std::string binaryPath;      // Path to the executable/jar
    std::string workingDir;      // Working directory for the process
    std::string extraOptions;    // Additional CLI flags
    std::map<std::string, std::string> env; // Environment variables
};

/**
 * Abstract interface for an assembler.
 */
class IAssembler {
public:
    virtual ~IAssembler() {}

    virtual const char* name() const = 0;
    virtual bool isaSupported(const std::string& isa) const = 0;

    /**
     * Set the backend configuration.
     */
    virtual void setConfig(const AssemblerConfig& config) { m_config = config; }
    virtual const AssemblerConfig& config() const { return m_config; }

    /**
     * Assemble a source file.
     * @param sourcePath Path to the input source file.
     * @param outputPath Path where the binary should be written.
     * @return Result structure with status and paths.
     */
    virtual AssemblerResult assemble(const std::string& sourcePath, const std::string& outputPath) = 0;

protected:
    AssemblerConfig m_config;
};
