/* $Id$ */

#ifndef ER_H
#define ER_H 1

#include <stdarg.h>

void error_begin(int error);
size_t error_appendf(const char* format, ...);
size_t error_vappendf(const char* format, va_list ap);
void error_end();

extern void (*error_callback)(const char* error_msg);

#endif
