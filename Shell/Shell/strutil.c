#include "strutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>


/*************************************************************************************
* 설명: 한 문자열의 시작을 입력받아서 앞쪽에 존재하는 공백을 제거한다.
* 인자:
* - begin: 문자열의 시작을 가리키는 포인터
* 반환값: trim이 적용된 문자열의 시작 포인터를 반환
*************************************************************************************/
char* strTrimFront(char* begin)
{
	char* ptr;
	for (ptr = begin; isspace(*ptr); ptr++);
	return ptr;
}

/*************************************************************************************
* 설명: 한 문자열의 시작과 끝을 입력받아서 양 끝에 존재하는 공백을 모두 제거한다.
*       중간에 있는 공백은 제거하지 않음. 범위는 [begin, end)
* 인자:
* - begin: 문자열의 시작을 가리키는 포인터
* - end: 문자열의 끝을 가리키는 포인터
* 반환값: trim이 적용된 문자열의 시작 포인터를 반환
*************************************************************************************/
char* strTrim(char* begin, char* end)
{
	if (begin >= end)
		return NULL;

	char* ptr;
	for (ptr = end - 1; isspace(*ptr) && begin < ptr; ptr--)
		*ptr = 0;
	for (ptr = begin; isspace(*ptr); ptr++);
	return ptr;
}

/*************************************************************************************
* 설명: 한 문자열의 시작과 끝을 입력받아서 양 끝 공백이 제거된 문자열을 dst에 복사한다.
* 인자:
* - dst: 복사한 문자열을 저장할 문자열
* - src_begin: 복사할 문자열의 시작을 가리키는 포인터
* - src_end: 복사할 문자열의 끝을 가리키는 포인터
* 반환값: 없음
*************************************************************************************/
void strTrimCopy(char* dst, char* src_begin, char* src_end)
{
	char* begin = src_begin;
	char* end = src_end;
	strParse(src_begin, &begin, &end);

	int len = end - begin;
	strncpy(dst, begin, len);
	dst[len] = 0;
}

void strParse(char* str, char** begin, char** end)
{
	if (str == NULL || *str == 0) {
		*begin = NULL;
		*end = NULL;
		return;
	}

	*begin = strTrimFront(str);
	for (*end = *begin; **end != ' ' && **end != '\t' && **end != '\n' && **end != 0; ++(*end)) {
		if (**end == ',') {
			++(*end);
			break;
		}
	}
}

int strToInt(char* str, int radix, BOOL* error) {
	char* end_ptr;
	int len = strlen(str);
	int value = strtol(str, &end_ptr, radix);
	if (error != NULL)
		*error = (BOOL)(end_ptr - str < len);
	return value;
}
