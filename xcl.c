#include "librnv.h"
#include <stdio.h>
#include <string.h>

#define ERR_MSG_BUF_LENGTH 4096

static int errctr = 0;
static char errmsg[ERR_MSG_BUF_LENGTH];

void print_usage() {
    printf("usage: rnv schema.rnc document.xml {other_documents.xml}");
}

void record_error(const char* error_msg) {
    errctr++;
    strcat_s(errmsg, ERR_MSG_BUF_LENGTH, error_msg);
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
