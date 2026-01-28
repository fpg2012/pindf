#include "logger.h"

void pindf_log(enum pindf_log_level level, const char *fmt, ...)
{
	const char *color_st = NULL;

	const char *level_str = NULL;
	switch (level) {
	case PINDF_LOG_LEVEL_DEBUG:
		level_str = "DEBUG";
		color_st = "\033[0;36m"; // cyan
		break;
	case PINDF_LOG_LEVEL_INFO:
		level_str = "INFO";
		color_st = "\033[0;34m"; // blue
		break;
	case PINDF_LOG_LEVEL_WARN:
		level_str = "WARN";
		color_st = "\033[0;33m"; // yellow
		break;
	case PINDF_LOG_LEVEL_ERROR:
		level_str = "ERROR";
		color_st = "\033[0;31m"; // red
		break;
	default:
		color_st = "\033[0;37m"; // gray
		level_str = "UNKNOWN";
		break;
	}

	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "%s[%s] ", color_st, level_str);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "%s\n", "\033[0m");
	va_end(args);
}