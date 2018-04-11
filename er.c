/* $Id$ */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "er.h"

#define MAX_ERR_MSG_LENGTH 2048
#define INVALID_ERROR_CODE INT_MAX

static int error_code = INVALID_ERROR_CODE;
static char error_msg[MAX_ERR_MSG_LENGTH];

static void default_error_print(const char* error_msg) {
    fputs(error_msg, stderr);
}

void (*error_callback)(const char*) = &default_error_print;

void error_begin(int error) {
    assert(error_code == INVALID_ERROR_CODE);
    error_code = error;
}

size_t error_appendf(const char* format, ...) {
    assert(error_code != INVALID_ERROR_CODE);
    size_t ret;
    va_list ap;
    va_start(ap, format);
    ret = error_vappendf(format, ap);
    va_end(ap);
    return ret;
}

size_t error_vappendf(const char* format, va_list ap) {
    size_t l = strlen(error_msg);
    return vsnprintf(error_msg + l, MAX_ERR_MSG_LENGTH - l, format, ap);
}

void error_end() {
    if(error_callback != NULL) {
        (*error_callback)(error_msg);
    }
    error_code = INVALID_ERROR_CODE;
    error_msg[0] = '\0';
}
