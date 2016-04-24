#ifndef ASMINSTRUCTION_H_
#define ASMINSTRUCTION_H_

#include <stdio.h>
#include "asmtype.h"

extern AsmInstruction* getInstruction(Assembler* asmblr, char* str);
extern void execInstStart(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstEnd(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstByte(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstWord(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstResb(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstResw(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstBase(Assembler* asmblr, Statement* stmt, FILE* log_stream);
extern void execInstOpcode(Assembler* asmblr, Statement* stmt, FILE* log_stream);

#endif