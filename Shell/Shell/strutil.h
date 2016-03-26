#ifndef STRUTIL_H_
#define STRUTIL_H_

#include <string.h>

#define strdup _strdup

extern char* strPartialDup(const char* start, const char* end);
extern char* strTrim(char* start, char* end);
extern char* strTrimDup(char* start, char* end);
extern char* strTrimCopy(char* start, char* end, char* dst);
extern char* strTokenDup(const char* str, const char* delimeter, char** save);
extern char* strParseDup(const char* str, char** save);

#endif
