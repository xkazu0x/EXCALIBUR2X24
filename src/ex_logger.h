#pragma once

#define EX_LOG_WARN_ENABLED 1

#ifdef EXCALIBUR_DEBUG
#define EX_LOG_INFO_ENABLED 1
#define EX_LOG_DEBUG_ENABLED 1
#endif

enum ex_log_level {
    EX_LOG_LEVEL_FATAL = 0,
    EX_LOG_LEVEL_ERROR = 1,
    EX_LOG_LEVEL_WARN = 2,
    EX_LOG_LEVEL_INFO = 3,
    EX_LOG_LEVEL_DEBUG = 4,
};

namespace ex::logger {
	void log_output(ex_log_level level, const char *message, ...);
}

#define EXFATAL(message, ...) ex::logger::log_output(EX_LOG_LEVEL_FATAL, message, ##__VA_ARGS__);
#define EXERROR(message, ...) ex::logger::log_output(EX_LOG_LEVEL_ERROR, message, ##__VA_ARGS__);

#if EX_LOG_WARN_ENABLED == 1
#define EXWARN(message, ...) ex::logger::log_output(EX_LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
#define EXWARN(message, ...)
#endif

#if EX_LOG_INFO_ENABLED == 1
#define EXINFO(message, ...) ex::logger::log_output(EX_LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
#define EXINFO(message, ...)
#endif

#if EX_LOG_DEBUG_ENABLED == 1
#define EXDEBUG(message, ...) ex::logger::log_output(EX_LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
#define EXDEBUG(message, ...)
#endif
