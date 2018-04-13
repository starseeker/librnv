#include "librnv.h"
#include <stdio.h>
#include <string.h>

#define ERR_MSG_BUF_LENGTH 4096

static int errctr = 0;
static char errmsg[ERR_MSG_BUF_LENGTH];

static size_t string_cat(char* dst, size_t dst_size, const char* src) {
#if defined _WIN32 // use strcat_s on Windows
    return strcat_s(dst, dst_size, src);
#elif !defined __GNU_LIBRARY && !defined __GLIBC__ // use strlcat for non GLIBC based UNIX systems (like BSD or Mac OS)
    return strlcat(dst, src, dst_size);
#else // use implementation below for GNU/Linux
    const char* s = src;
    char* d = dst;
    size_t n = dst_size;
    size_t dst_length;

    while(n-- != 0 && *d != '\0') d++;
    dst_length = d - dst;
    n = dst_size - dst_length;

    if(n == 0) {
        return(dst_length + strlen(s));
    }
    while(*s != '\0') {
        if(n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return(dst_length + (s - src));
#endif
}

void print_usage() {
    printf("usage: rnv schema.rnc document.xml {other_documents.xml}");
}

void record_error(const char* error_msg) {
    errctr++;
    string_cat(errmsg, ERR_MSG_BUF_LENGTH, error_msg);
}

int main(int argc, char** argv) {
    if(argc < 3) {
        print_usage();
        return -1;
    }

    memset(errmsg, 0, ERR_MSG_BUF_LENGTH);

    rnv_initialize();
    rnv_set_error_callback(record_error);

    int i = 1; // argument index
    if(rnv_load_schema(argv[i++]) != RNV_ERR_NO) {
        printf("Invalid Relax NG schema:\n%s", errmsg);
        return errctr;
    }
    while(i < argc) {
        const char* xml_document = argv[i++];
        printf("%s -> ", xml_document);
        if(rnv_validate(xml_document) != RNV_ERR_NO) {
            printf("validation failed with %i error(s):\n%s", errctr, errmsg);
        }
        else {
            printf("valid\n");
        }
    }

    rnv_cleanup();

    return errctr;
}
