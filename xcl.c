#include "librnv.h"
#include <stdio.h>

int print_error() {
    rnv_error error = rnv_get_last_error();
    printf("%s\n", error.msg);
    return error.code;
}

void print_usage() {
    printf("usage: rnv schema.rnc document.xml {other_documents.xml}");
}

int main(int argc, char** argv) {
    int error = RNV_ERR_NO;
    if(argc < 3) {
        print_usage();
        return -1;
    }
    int i = 1;
    rnv_initialize();
    error = rnv_load_schema(argv[i++]);
    if(error) {
        printf("Invalid Relax NG schema:\n");
        return print_error();
    }
    while(i < argc) {
        const char* xml_document = argv[i++];
        printf("%s -> ", xml_document);
        error = rnv_validate(xml_document);
        if(error) {
            printf("validation failure:\n");
            return print_error();
        }
        else {
            printf("valid\n");
        }
    }
    return error;
}
