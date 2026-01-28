#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define PINDF_ERR(...) \
	do { \
		pindf_log(PINDF_LOG_LEVEL_ERROR, __VA_ARGS__); \
	} while (0)
#define PINDF_WARN(...) \
	do { \
		pindf_log(PINDF_LOG_LEVEL_WARN, __VA_ARGS__); \
	} while (0)
#define PINDF_INFO(...) \
	do { \
		pindf_log(PINDF_LOG_LEVEL_INFO, __VA_ARGS__); \
	} while (0)
#define PINDF_DEBUG(...) \
	do { \
		pindf_log(PINDF_LOG_LEVEL_DEBUG, __VA_ARGS__); \
	} while (0)

enum pindf_log_level {
	PINDF_LOG_LEVEL_DEBUG = 0,
	PINDF_LOG_LEVEL_INFO,
	PINDF_LOG_LEVEL_WARN,
	PINDF_LOG_LEVEL_ERROR,
};

void pindf_log(enum pindf_log_level level, const char *fmt, ...);