#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

char* strPartialDup(const char* start, const char* end)
{
	int len = (int)(end - start);
	char* dup = (char*)calloc(len + 1, sizeof(char));
	strncpy(dup, start, len);
	return dup;
}

/*************************************************************************************
* 설명: 한 문자열의 시작을 입력받아서 앞쪽에 존재하는 공백을 제거한다.
* 인자:
* - start: 문자열의 시작을 가리키는 포인터
* 반환값: trim이 적용된 문자열의 시작 포인터를 반환
*************************************************************************************/
char* strTrimFront(char* start)
{
	char* ptr;
	for (ptr = start; isspace(*ptr); ptr++);
	return ptr;
}

/*************************************************************************************
* 설명: 한 문자열의 시작과 끝을 입력받아서 양 끝에 존재하는 공백을 모두 제거한다.
*       중간에 있는 공백은 제거하지 않음.
* 인자:
* - start: 문자열의 시작을 가리키는 포인터
* - end: 문자열의 끝을 가리키는 포인터
* 반환값: trim이 적용된 문자열의 시작 포인터를 반환
*************************************************************************************/
char* strTrim(char* start, char* end)
{
	char* ptr0;
	char* ptr1;

	for (ptr0 = start; isspace(*ptr0); ptr0++);
	for (ptr1 = end - 1; isspace(*ptr1); ptr1--)
		*ptr1 = 0;

	return ptr0;
}

/*************************************************************************************
* 설명: 한 문자열의 시작과 끝을 입력받아서 양 끝에 존재하는 공백을 모두 제거한다.
*       중간에 있는 공백은 제거하지 않음.
* 인자:
* - start: 문자열의 시작을 가리키는 포인터
* - end: 문자열의 끝을 가리키는 포인터
* 반환값: trim이 적용된 문자열을 복사하여 새로 할당하고, 그 주소값을 반환
*************************************************************************************/
char* strTrimDup(char* start, char* end)
{
	char* ptr0;
	char* ptr1;
	char* newStr;
	int len;

	for (ptr0 = start; isspace(*ptr0); ptr0++);
	for (ptr1 = end - 1; isspace(*ptr1); ptr1--);

	len = (int)(ptr1 - ptr0) + 1;
	newStr = (char*)malloc(sizeof(char) * (len + 1));
	if (newStr == NULL)
		return NULL;

	strncpy(newStr, ptr0, len);
	newStr[len] = 0;

	return newStr;
}

/*************************************************************************************
* 설명: 한 문자열의 시작과 끝을 입력받아서 양 끝에 존재하는 공백을 모두 제거한다.
*       중간에 있는 공백은 제거하지 않음.
* 인자:
* - start: 문자열의 시작을 가리키는 포인터
* - end: 문자열의 끝을 가리키는 포인터
* 반환값: trim이 적용된 문자열을 dst에 복사하고 dst를 반환
*************************************************************************************/
char* strTrimCopy(char* start, char* end, char* dst)
{
	char* ptr0;
	char* ptr1;
	int len;

	for (ptr0 = start; isspace(*ptr0); ptr0++);
	for (ptr1 = end - 1; isspace(*ptr1); ptr1--);

	len = (int)(ptr1 - ptr0) + 1;
	strncpy(dst, ptr0, len);
	dst[len] = 0;

	return dst;
}

char* strTokenDup(const char* str, const char* delimeter, char** save)
{
	if (delimeter == NULL) {
		return strdup(str);
	}
	else if (str != NULL) {
		return NULL;
	}
	else if (save != NULL) {
		return NULL;
	}
	else {
		return NULL;
	}
}

char* strParseDup(const char* str, char** save)
{
	char* start;
	if (str != NULL)
		start = strTrimFront(str);
	else if (save != NULL)
		start = strTrimFront(*save);
	else
		return NULL;

	*save = start;
	for (*save = start; **save != ' ' && **save != '\t' && **save != '\n' && **save != 0; ++(*save)) {
		if (**save == ',') {
			++(*save);
			break;
		}
	}
	return strPartialDup(start, *save);
}