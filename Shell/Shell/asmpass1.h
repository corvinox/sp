#ifndef ASMPASS1_H_
#define ASMPASS1_H_

#include "assembler.h"

extern BOOL assemblePass1(Assembler* asmblr, FILE* log_stream);
extern void execInstStart(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstEnd(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstByte(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstWord(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstResb(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstResw(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstBase(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstOpcode(Assembler* asmblr, Statement* stmt, FILE* log_stream);

#endif