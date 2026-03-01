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
#include <assert.h>

#define PINDF_MIN(a, b) ((a) < (b) ? (a) : (b))
#define PINDF_MAX(a, b) ((a) > (b) ? (a) : (b))
#define PINDF_MAX_DEREF_ITER 100

typedef unsigned char uchar;
typedef unsigned int uint;
typedef long long int64;
typedef unsigned long long uint64;

enum pindf_errno {
	PINDF_OK = 0,
	PINDF_EOF_ERR = -1,
	PINDF_GNR_ERR = -2,
	PINDF_MEM_ERR = -3
};