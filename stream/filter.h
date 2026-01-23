#pragma once

#include <zlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../container/uchar_str.h"

#define PINDF_MAX_EXPAND_RETRY 4

enum pindf_filter_errno {
	PINDF_FLTR_OK = 0,
	PINDF_FLTR_MEM_ERR = -1,
	PINDF_FLTR_DAT_ERR = -2,
};

int pindf_zlib_uncompress(pindf_uchar_str *dest, pindf_uchar_str *src);
int pindf_zlib_compress(pindf_uchar_str *dest, pindf_uchar_str *src);