#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <spdlog/spdlog.h>
#include "include/mmemu_plugin_api.h"

/**
 * Host-side registry for named spdlog loggers.
 *
 * This class lives exclusively in the host binary (libplugins.a).  Plugins
 * must never link against it directly; they obtain and use loggers only
 * through the SimPluginHostAPI function pointers (getLogger / logNamed).
 *
 * All logger creation therefore routes through getLogger(), so m_loggers is
 * the single authoritative source of truth for log list / log level commands.
 */
class LogRegistry {
public:
    static LogRegistry& instance();

    /** Get or create a named logger backed by the shared sink set. */
    std::shared_ptr<spdlog::logger> getLogger(const std::string& name);

    /** Names of every logger registered so far. */
    std::vector<std::string> getLoggerNames() const;

    /** Set the level of every registered logger. */
    void setGlobalLevel(spdlog::level::level_enum level);

    /** Maps SimLogLevel enum values to spdlog::level. */
    static spdlog::level::level_enum mapLevel(int simLevel);

    /**
     * Initialise sinks and ensure the "system" logger exists.
     * Safe to call multiple times (idempotent).  Must be called from main()
     * before loadFromDir() so that "log list" is never empty.
     */
    void init();

private:
    LogRegistry() = default;
    std::map<std::string, std::shared_ptr<spdlog::logger>> m_loggers;
    std::vector<spdlog::sink_ptr> m_sinks;
};
