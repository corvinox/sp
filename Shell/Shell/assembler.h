#ifndef ASSEBLER_H_
#define ASSEBLER_H_

#include <stdio.h>
#include "asmtype.h"

extern BOOL assemblerInitialize(Assembler* asmblr);
extern void assemblerRelease(Assembler* asmblr);
extern BOOL assemblerIsIntialized(Assembler* asmblr);
extern void assemblerAssemble(Assembler* asmblr, const char* filename, FILE* log_stream);
extern void assemblerPrintOpcode(Assembler* asmblr, char* opcode, FILE* stream);
extern void assemblerPrintOpcodeTable(Assembler* asmblr, FILE* stream);
extern void assemblerPrintSymbolTable(Assembler* asmblr, FILE* stream);

#endif