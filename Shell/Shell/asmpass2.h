#ifndef ASMPASS2_H_
#define ASMPASS2_H_

#include "assembler.h"

typedef struct _HeaderRecord
{
	int prog_addr;
	int prog_len;
	char prog_name[7];

} HeaderRecord;

typedef struct _TextRecord
{
	int obj_addr;
	int obj_len;
	char obj_code[7];

} TextRecord;

typedef struct _EndRecord
{
	int executable;
} EndRecord;

typedef struct _ModificationRecord
{
	int start_addr;
	int half_bytes;
} ModificationRecord;

extern BOOL assemblePass2(Assembler* asmblr, FILE* log_stream);

#endif