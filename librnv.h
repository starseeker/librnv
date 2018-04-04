#ifndef LIBRNV_H
#define LIBRNV_H

#if defined (WIN32)
#if defined(librnv_EXPORTS)
#define  RNVAPI __declspec(dllexport)
#else
#define  RNVAPI __declspec(dllimport)
#endif
#else
#define RNVAPI
#endif

#define RNV_ERR_MAXLEN 4096
#define RNV_PATH_MAXLEN 512

#define RNV_ERR_NO               0
#define RNV_ERR_UNKOWN           1
#define RNV_ERR_ALLOCFAILURE     2
#define RNV_ERR_FILEIO           3
#define RNV_ERR_INVALIDSCHEMA    4
#define RNV_ERR_INVALIDXML       5
#define RNV_ERR_VALIDATIONFAILED 6

#ifdef __cplusplus
extern "C" {
#endif

RNVAPI typedef struct _rnv_error {
	int code;
	const char* msg;
} rnv_error;

RNVAPI void rnv_initialize();
RNVAPI void rnv_cleanup();

RNVAPI int rnv_load_schema(const char* rnc_file_path);
RNVAPI int rnv_validate(const char* xml_file_path);
RNVAPI void rnv_reset();

RNVAPI rnv_error rnv_get_last_error();

#ifdef __cplusplus
}
#endif

#endif // LIBRNV_H
