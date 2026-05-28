#ifndef GOODBYEBIGSLOW_FLAG_UTILS_H
#define GOODBYEBIGSLOW_FLAG_UTILS_H

#ifndef KERNEL
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#endif

static inline bool eql_flag(const char *a, const char *b, size_t n)
{
    while (n > 0 && *a && *b) {
        if (*a != *b || *a == ':' || *b == ':') return false;
        ++a; ++b; --n;
    }
    return n == 0;
}

static inline bool has_flag(const char *args, const char *arg)
{
    if (arg[0] == '-' || arg[0] == '+') {
        size_t n = strlen(arg);
        for (const char *p = args; *p; ++p) {
            if ((p == args || p[-1] == ':') && (p[n] == 0 || p[n] == ':')
                    && eql_flag(p, arg, n)) {
                return true;
            }
        }
    }
    return false;
}

#endif // GOODBYEBIGSLOW_FLAG_UTILS_H
