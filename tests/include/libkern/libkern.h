#ifndef LIBKERN_LIBKERN_H
#define LIBKERN_LIBKERN_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef int64_t SInt64;

bool PE_parse_boot_argn(const char *argName, void *argVal, int maxLen);

#endif
