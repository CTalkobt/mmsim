#include "include/util/logging.h"
#include <spdlog/sinks/stdout_color_sinks.h>

LogRegistry& LogRegistry::instance() {
    static LogRegistry s_instance;
    return s_instance;
}

void LogRegistry::init() {
    if (m_sinks.empty()) {
        m_sinks.push_back(std::make_shared<spdlog::sinks::stderr_color_sink_mt>());
        spdlog::set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
        
        // Ensure system logger exists
        getLogger("system");
    }
}

std::shared_ptr<spdlog::logger> LogRegistry::getLogger(const std::string& name) {
    auto it = m_loggers.find(name);
    if (it != m_loggers.end()) return it->second;

    init();
    auto logger = std::make_shared<spdlog::logger>(name, m_sinks.begin(), m_sinks.end());
    logger->set_level(spdlog::level::info);
    m_loggers[name] = logger;
    return logger;
}

std::vector<std::string> LogRegistry::getLoggerNames() const {
    std::vector<std::string> names;
    for (auto const& [name, _] : m_loggers) {
        names.push_back(name);
    }
    return names;
}

void LogRegistry::setGlobalLevel(spdlog::level::level_enum level) {
    for (auto& [_, logger] : m_loggers) {
        logger->set_level(level);
    }
}

spdlog::level::level_enum LogRegistry::mapLevel(int simLevel) {
    switch (simLevel) {
        case SIM_LOG_TRACE:    return spdlog::level::trace;
        case SIM_LOG_DEBUG:    return spdlog::level::debug;
        case SIM_LOG_INFO:     return spdlog::level::info;
        case SIM_LOG_WARN:     return spdlog::level::warn;
        case SIM_LOG_ERROR:    return spdlog::level::err;
        case SIM_LOG_CRITICAL: return spdlog::level::critical;
        case SIM_LOG_OFF:      return spdlog::level::off;
        default:               return spdlog::level::info;
    }
}
