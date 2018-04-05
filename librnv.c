/* $Id$ */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include EXPAT_H
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

#define PIXGFILE "davidashen-net-xg-file"
#define PIXGPOS "davidashen-net-xg-pos"

static int nexp;
static char* xml;
static XML_Parser expat = NULL;
static int start,current,previous;
static int mixed=0;
static int lastline,lastcol,level;
static char* xgfile = NULL;
static char* xgpos = NULL;
static int ok;

/* Expat does not normalize strings on input */
static char* text; static int len_txt;
static int n_txt;

#define err(msg) (*er_vprintf)(msg"\n",ap);
static void verror_handler(int erno,va_list ap) {
  if(erno&ERBIT_RNL) {
    rnl_default_verror_handler(erno&~ERBIT_RNL,ap);
  } else {
    int line=XML_GetCurrentLineNumber(expat),col=XML_GetCurrentColumnNumber(expat);
    if(line!=lastline||col!=lastcol) { lastline=line; lastcol=col;
      if(xgfile) (*er_printf)("%s:%s: error: ",xgfile,xgpos); else
      (*er_printf)("%s:%i:%i: error: ",xml,line,col);
      if(erno&ERBIT_RNV) {
	rnv_default_verror_handler(erno&~ERBIT_RNV,ap);
	if(nexp) { int req=2, i=0; char *s;
	  while(req--) {
	    rnx_expected(previous,req);
	    if(i==rnx_n_exp) continue;
	    if(rnx_n_exp>nexp) break;
	    (*er_printf)((char*)(req?"required:\n":"allowed:\n"));
	    for(;i!=rnx_n_exp;++i) {
	      (*er_printf)("\t%s\n",s=rnx_p2str(rnx_exp[i]));
	      m_free(s);
	    }
	  }
	}
      } else {
	switch(erno) {
	case XCL_ER_IO: err("%s"); break;
	case XCL_ER_XML: err("%s"); break;
	case XCL_ER_XENT: err("pipe through xx to expand external entities"); break;
	default: assert(0);
	}
      }
    }
  }
}

static void verror_handler_rnl(int erno,va_list ap) {verror_handler(erno|ERBIT_RNL,ap);}
static void verror_handler_rnv(int erno,va_list ap) {verror_handler(erno|ERBIT_RNV,ap);}

static void windup(void);
static int initialized=0;
static void init(void) {
  if(!initialized) {initialized=1;
    rnl_init(); rnl_verror_handler=&verror_handler_rnl;
    rnv_init(); rnv_verror_handler=&verror_handler_rnv;
    rnx_init();
    text=(char*)m_alloc(len_txt=LEN_T,sizeof(char));
    windup();
  }
}

static void clear(void) {
  if(len_txt>LIM_T) {m_free(text); text=(char*)m_alloc(len_txt=LEN_T,sizeof(char));}
  windup();
}

static void windup(void) {
  text[n_txt=0]='\0';
  level=0; lastline=lastcol=-1;
}

static void error_handler(int erno,...) {
  va_list ap; va_start(ap,erno); verror_handler(erno,ap); va_end(ap);
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

static void processingInstruction(void *userData,
    const char *target,const char *data) {
  if(strcmp(PIXGFILE,target)==0) {
    if(xgfile) m_free(xgfile); 
    xgfile=s_clone((char*)data);
  } else if(strcmp(PIXGPOS,target)==0) {
    if(xgpos) m_free(xgpos);
    xgpos=s_clone((char*)data);
    *strchr(xgpos,' ')=':';
  }
}

static int process(FILE* fp) {
  void *buf; size_t len;
  for(;;) {
    buf=XML_GetBuffer(expat,BUFSIZE);
    len = fread(buf, 1, BUFSIZE, fp);
    if(len<0) {
      error_handler(XCL_ER_IO,xml,strerror(errno));
      goto ERROR;
    }
    if(!XML_ParseBuffer(expat, (int)len, len == 0)) goto PARSE_ERROR;
    if(len==0) break;
  }
  return ok;

PARSE_ERROR:
  error_handler(XCL_ER_XML,XML_ErrorString(XML_GetErrorCode(expat)));
ERROR:
  return 0;
}

static int externalEntityRef(XML_Parser p,const char *context,
    const char *base,const char *systemId,const char *publicId) {
  error_handler(XCL_ER_XENT);
  return 1;
}

static void validate(FILE* fp) {
  previous=current=start;
  expat=XML_ParserCreateNS(NULL,':');
  XML_SetParamEntityParsing(expat,XML_PARAM_ENTITY_PARSING_ALWAYS);
  XML_SetElementHandler(expat,&start_element,&end_element);
  XML_SetCharacterDataHandler(expat,&characters);
  XML_SetExternalEntityRefHandler(expat,&externalEntityRef);
  XML_SetProcessingInstructionHandler(expat,&processingInstruction);
  ok=process(fp);
  XML_ParserFree(expat);
}

rnv_error last_error;
char rnc_path[RNV_PATH_MAXLEN];
char err_msg[RNV_ERR_MAXLEN];

int er_string_printf(char *format, ...) {
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = (*er_vprintf)(format, ap);
	va_end(ap);
	return ret;
}
int er_string_vprintf(char *format, va_list ap) {
	if(last_error.code == RNV_ERR_NO) last_error.code = RNV_ERR_UNKOWN;
    size_t l = strlen(err_msg);
    return vsnprintf(err_msg + l, RNV_ERR_MAXLEN - l, format, ap);
}

void rnv_initialize() {
	init();

	nexp = NEXP;

	rnv_reset();

	er_printf = &er_string_printf;
	er_vprintf = &er_string_vprintf;
}

void rnv_cleanup() {
	clear();
}

int rnv_load_schema(const char* rnc_file_path) {
    strcpy(rnc_path, rnc_file_path);
	ok = start = rnl_fn(rnc_path);

	last_error.code = ok ? RNV_ERR_NO : RNV_ERR_INVALIDSCHEMA;
	return last_error.code;
}

int rnv_validate(const char* xml_file_path) {
    FILE* fp = fopen(xml_file_path, "r");
    if(fp == NULL) {
		(*er_printf)("I/O error (%s): %s\n", xml_file_path, strerror(errno));
		return last_error.code = RNV_ERR_FILEIO;
	}
    validate(fp);
    fclose(fp);
	clear();
	return last_error.code;
}

void rnv_reset() {
	clear();
	memset(rnc_path, 0, RNV_PATH_MAXLEN);
	memset(err_msg, 0, RNV_ERR_MAXLEN);
	last_error.code = RNV_ERR_NO;
	last_error.msg = err_msg;
}

rnv_error rnv_get_last_error() {
	return last_error;
}
