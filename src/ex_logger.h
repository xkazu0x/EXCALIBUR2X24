#pragma once

#define LOG_WARN_ENABLED 1

#ifdef EXCALIBUR_DEBUG
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#endif

enum log_level {
	LOG_LEVEL_FATAL = 0,
	LOG_LEVEL_ERROR = 1,
	LOG_LEVEL_WARN = 2,
	LOG_LEVEL_INFO = 3,
	LOG_LEVEL_DEBUG = 4,
};

namespace ex::logger {
	void log_output(log_level level, const char *message, ...);
}

#define EXFATAL(message, ...) ex::logger::log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);
#define EXERROR(message, ...) ex::logger::log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);

#if LOG_WARN_ENABLED == 1
#define EXWARN(message, ...) ex::logger::log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
#define EXWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
#define EXINFO(message, ...) ex::logger::log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
#define EXINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
#define EXDEBUG(message, ...) ex::logger::log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
#define EXDEBUG(message, ...)
#endif
