#ifndef IOLIB_H
#define IOLIB_H

#include <stdarg.h>
#include <stdbool.h>

void IOLog(const char *format, ...);
void IOSleep(uint32_t milliseconds);

bool PE_parse_boot_argn(const char *arg_name, void *arg_ptr, int max_len);

void mp_rendezvous_no_intrs(void (*func)(void *), void *arg);

#endif
