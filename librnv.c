/* $Id$ */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <expat.h>
#include "m.h"
#include "s.h"
#include "erbit.h"
#include "drv.h"
#include "rnl.h"
#include "rnv.h"
#include "rnx.h"
#include "ll.h"
#include "er.h"
#include "librnv.h"

extern int rn_notAllowed,rx_compact,drv_compact;

#define LEN_T XCL_LEN_T
#define LIM_T XCL_LIM_T

#define BUFSIZE 1024

/* maximum number of candidates to display */
#define NEXP 16

#define XCL_ER_IO 0
#define XCL_ER_XML 1
#define XCL_ER_XENT 2

static int nexp;
static char* xml;
static XML_Parser expat = NULL;
static int start,current,previous;
static int mixed=0;
static int lastline,lastcol,level;
static char* xgfile = NULL;
static char* xgpos = NULL;
static int ok;
static int initialized = 0;

/* Expat does not normalize strings on input */
static char* text; static int len_txt;
static int n_txt;

static void validation_error_handler(int erno, va_list ap) {
    int line = XML_GetCurrentLineNumber(expat);
    int col = XML_GetCurrentColumnNumber(expat);
    if(line != lastline || col != lastcol) {
        lastline = line;
        lastcol = col;
        error_begin(erno);
        error_appendf("%s:%i:%i: error: ", xml, line, col);
        rnv_default_verror_handler(erno, ap);
        if(nexp) {
            int req = 2;
            int i = 0;
            char* s;
            while(req--) {
                rnx_expected(previous, req);
                if(i == rnx_n_exp) continue;
                if(rnx_n_exp > nexp) break;
                error_appendf((const char*)(req ? "required:\n" : "allowed:\n"));
                for(; i != rnx_n_exp; ++i) {
                    error_appendf("\t%s\n",s=rnx_p2str(rnx_exp[i]));
                    m_free(s);
                }
            }
        }
        error_end();
    }
}

static void schema_error_handler(int erno, va_list ap) {
  rnl_default_verror_handler(erno, ap);
}

static void local_error_handler(int erno, va_list ap) {
    error_begin(erno);
    switch(erno) {
        case XCL_ER_IO: error_vappendf("%s", ap); break;
        case XCL_ER_XML: error_vappendf("%s", ap); break;
        case XCL_ER_XENT: error_vappendf("external entities are not supported", ap); break;
        default: assert(0);
    }
    error_end();
}

static void report_error(int erno, ...) {
  va_list ap;
  va_start(ap, erno);
  local_error_handler(erno, ap);
  va_end(ap);
}

static void windup(void) {
    n_txt = 0;
    text[n_txt] = '\0';
    level = 0;
    lastline = -1;
    lastcol = -1;
}

static void clear(void) {
  if(len_txt>LIM_T) {m_free(text); text=(char*)m_alloc(len_txt=LEN_T,sizeof(char));}
  windup();
}

static void flush_text(void) {
  ok=rnv_text(&current,&previous,text,n_txt,mixed)&&ok;
  text[n_txt=0]='\0';
}

static void start_element(void *userData,const char *name,const char **attrs) {
  if(current!=rn_notAllowed) {
    mixed=1;
    flush_text();
    ok=rnv_start_tag(&current,&previous,(char*)name,(char**)attrs)&&ok;
    mixed=0;
  } else {
    ++level;
  }
}

static void end_element(void *userData,const char *name) {
  if(current!=rn_notAllowed) {
    flush_text();
    ok=rnv_end_tag(&current,&previous,(char*)name)&&ok;
    mixed=1;
  } else {
    if(level==0) current=previous; else --level;
  }
}

static void characters(void *userData,const char *s,int len) {
  if(current!=rn_notAllowed) {
    int newlen_txt=n_txt+len+1;
    if(newlen_txt<=LIM_T&&LIM_T<len_txt) newlen_txt=LIM_T;
    else if(newlen_txt<len_txt) newlen_txt=len_txt;
    if(len_txt!=newlen_txt) text=(char*)m_stretch(text,len_txt=newlen_txt,n_txt,sizeof(char));
    memcpy(text+n_txt,s,len); n_txt+=len; text[n_txt]='\0'; /* '\0' guarantees that the text is bounded, and strto[ld] work for data */
  }
}

static int process(FILE* fp) {
  void *buf; size_t len;
  for(;;) {
    buf=XML_GetBuffer(expat,BUFSIZE);
    len = fread(buf, 1, BUFSIZE, fp);
    if(len<0) {
      report_error(XCL_ER_IO, xml, strerror(errno));
      goto ERROR;
    }
    if(!XML_ParseBuffer(expat, (int)len, len == 0)) goto PARSE_ERROR;
    if(len==0) break;
  }
  return ok;

PARSE_ERROR:
  report_error(XCL_ER_XML, XML_ErrorString(XML_GetErrorCode(expat)));
ERROR:
  return 0;
}

static int externalEntityRef(XML_Parser p,const char *context,
    const char *base,const char *systemId,const char *publicId) {
  report_error(XCL_ER_XENT);
  return 1;
}

static void validate(FILE* fp) {
  previous=current=start;
  expat=XML_ParserCreateNS(NULL,':');
  XML_SetParamEntityParsing(expat,XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetElementHandler(expat,&start_element,&end_element);
  XML_SetCharacterDataHandler(expat,&characters);
  XML_SetExternalEntityRefHandler(expat,&externalEntityRef);
  ok=process(fp);
  XML_ParserFree(expat);
}

void rnv_initialize() {
    if(!initialized) {
        initialized = 1;

        rnl_init();
        rnl_verror_handler = &schema_error_handler;

        rnv_init();
        rnv_verror_handler = &validation_error_handler;

        rnx_init();

        text = (char*)m_alloc(len_txt = LEN_T, sizeof(char));
        windup();

        nexp = 0;
    }
}

void rnv_set_display_candidates(int number) {
    nexp = number;
}

void rnv_set_error_callback(void (*callback)(const char*)) {
    error_callback = callback;
}

void rnv_cleanup() {
    clear(); // TODO ???
    free(text);

    initialized = 0;
}

/**
 * @brief Load a Relax NG compact syntax schema
 * @param rnc_file_path Path to the schema file
 * @return RNV_ERR_NO on success or an error code otherwise
 */
int rnv_load_schema(const char* rnc_file_path) {
    ok = start = rnl_fn(rnc_file_path);
    return ok ? RNV_ERR_NO : RNV_ERR_INVALIDSCHEMA;
}

/**
 * @brief rnv_validate
 * @param xml_file_path
 * @return
 */
int rnv_validate(const char* xml_file_path) {
    xml = xml_file_path;
    FILE* fp = fopen(xml, "r");
    if(fp == NULL) {
        report_error(XCL_ER_IO, xml, strerror(errno));
        return RNV_ERR_FILEIO;
	}
    validate(fp);
    fclose(fp);
	clear();
    xml = NULL;
    return ok ? RNV_ERR_NO : RNV_ERR_VALIDATIONFAILED;
}
