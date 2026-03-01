/*
 Copyright 2026 fpg2012 (aka nth233)

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#define PINDF_DEFAULT_LEVEL PINDF_LOG_LEVEL_INFO

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
void pindf_set_log_level(enum pindf_log_level level);