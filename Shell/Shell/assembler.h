#ifndef ASSEBLER_H_
#define ASSEBLER_H_

#include "mytype.h"
#include "hash.h"

#define REG_CNT 9

typedef enum
{
	ERR_NO = 0,
	ERR_NO_INIT,
	ERR_DUPLICATE_SYMBOL,
	ERR_UNDEFINED_SYMBOL,
	ERR_INVALID_OPCODE,
} AsmError;

typedef enum
{
	INST_START = 0,
	INST_END,
	INST_BYTE,
	INST_WORD,
	INST_RESB,
	INST_RESW,
	INST_OPCODE
} AsmInst;

typedef enum
{
	ADDR_SIMPLE = 0,
	ADDR_IMMEDIATE,
	ADDR_INDIRECT
} AddrMode;

typedef struct
{
	int line_number;
	int loc;
	int error;
	int addr_mode;
	char* label;
	char* instruction;
	char* operand;
	char* comment;
	BOOL is_extended;
	BOOL is_comment;
	BOOL has_label;
	BOOL has_operand;
} Statement;

typedef struct
{
	int line_number;
	int loc;
	int inst;
	int addr_mode;
} IntStatement;

typedef struct
{
	char* mnemonic;
	int number;
} Register;

typedef struct
{
	int error;
	int prog_len;
	int start_addr;
	int locctr;
	HashTable op_table;
	HashTable sym_table;
	Register reg_table[REG_CNT];
} Assembler;

extern void assemblerInitialize(Assembler* asmblr);
extern void assemblerAssemble(Assembler* asmblr, const char* filename);
extern void assemblerRelease(Assembler* asmblr);
extern BOOL assemblerIsInitialized(Assembler* asmblr);

#endif