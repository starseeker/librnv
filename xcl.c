#include "librnv.h"
#include <stdio.h>
#include <string.h>

#define ERR_MSG_BUF_LENGTH 4096

static int errctr = 0;
static char errmsg[ERR_MSG_BUF_LENGTH];

void print_usage() {
    printf("usage: rnv schema.rnc document.xml {other_documents.xml}");
}

int record_error(char* format, va_list ap) {
    errctr++;
    size_t l = strlen(errmsg);
    return vsnprintf(errmsg + l, RNV_ERR_MAXLEN - l, format, ap);
}

int main(int argc, char** argv) {
    if(argc < 3) {
        print_usage();
        return -1;
    }

    memset(errmsg, 0, ERR_MSG_BUF_LENGTH);

    rnv_initialize();
    rnv_set_error_printf(record_error);

    int i = 1; // argument index
    if(rnv_load_schema(argv[i++]) != RNV_ERR_NO) {
        printf("Invalid Relax NG schema\n");
        return -2;
    }
    while(i < argc) {
        const char* xml_document = argv[i++];
        printf("%s -> ", xml_document);
        if(rnv_validate(xml_document) != RNV_ERR_NO) {
            printf("validation failure:\n%s", errmsg);
        }
        else {
            printf("valid\n");
        }
    }

    rnv_cleanup();

    return errctr;
}
