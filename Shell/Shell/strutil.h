#ifndef STRUTIL_H_
#define STRUTIL_H_

#include <string.h>
#include "mytype.h"

#define strdup _strdup

extern char* strTrimFront(char* begin);
extern char* strTrimBack(char* end);
extern char* strTrim(char* begin, char* end);
extern void strTrimCopy(char* dst, char* begin, char* end);
extern void strParse(char* str, char** begin, char** end);
extern int strToInt(char* str, int radix, BOOL* error);
extern BOOL isHexadecimalStr(char* str);
extern BOOL isDecimalStr(char* str);

#endif
