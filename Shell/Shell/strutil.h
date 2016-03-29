#ifndef STRUTIL_H_
#define STRUTIL_H_

#include <string.h>
#include "mytype.h"

#define strdup _strdup

extern char* strPartialDup(char* start, char* end);
extern char* strTrim(char* start, char* end);
extern char* strTrimDup(char* start, char* end);
extern char* strTrimCopy(char* start, char* end, char* dst);
extern char* strTokenDup(char* str, char* delimeter, char** save);
extern void strParse(char* str, char** begin, char** end);
extern char* strParseDup(char* str, char** save);
extern BOOL isHexadecimalStr(char* str);
extern BOOL isDecimalStr(char* str);

#endif
