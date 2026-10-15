#pragma once

#include "lib/common.h"

struct sbiret {
    long error;  // Return error code (mapped from register a0)
    long value;  // Return value/data (mapped from register a1)
};

struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid);
void putchar(char c);
long getchar(void);
