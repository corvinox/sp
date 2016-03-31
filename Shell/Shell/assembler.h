#ifndef ASSEBLER_H_
#define ASSEBLER_H_

#include <stdio.h>
#include "mytype.h"
#include "hash.h"

#define COMMENT_LEN     1024
#define LABEL_LEN_MAX   8
#define INSTRUCTION_LEN 8
#define OPERAND_LEN     128
#define FILENAME_LEN_MAX 16

typedef void (*ExecFuncPtr)(struct _Assembler*, struct _Statement*, FILE*);

typedef enum _AsmErrorCode
{
	ERR_NO,
	ERR_NO_INIT,
	ERR_DUPLICATE_SYMBOL,
	ERR_UNDEFINED_SYMBOL,
	ERR_INVALID_OPCODE,
} AsmErrorCode;

typedef enum _AsmInstCode
{
	INST_NO,
	INST_START,
	INST_END,
	INST_BYTE,
	INST_WORD,
	INST_RESB,
	INST_RESW,
	INST_BASE,
	INST_OPCODE
} AsmInstCode;

typedef enum _AddrMode
{
	ADDR_SIMPLE = 0,
	ADDR_IMMEDIATE,
	ADDR_INDIRECT
} AddrMode;

typedef struct _Statement
{
	int line_number;
	int loc;
	int inst_code;
	int addr_mode;
	int error;
	BOOL is_invalid;
	BOOL is_empty;
	BOOL is_extended;
	BOOL is_indexed;
	BOOL is_comment;
	BOOL has_label;
	BOOL has_operand;

	struct _AsmInstruction* instruction;
	char label[LABEL_LEN_MAX];
	char operand[OPERAND_LEN];
} Statement;

typedef struct _IntStatement
{
	int line_number;
	int loc;
	int inst;
	int addr_mode;
} IntStatement;

typedef struct _Register
{
	char* mnemonic;
	int number;
} AsmRegister;

typedef struct _AsmInstruction
{
	char* mnemonic;
	int value;
	void* aux;
	ExecFuncPtr exec_func;
} AsmInstruction;

typedef struct _Assembler
{
	int error;
	int prog_len;
	int start_addr;
	int locctr;

	HashTable dir_table;
	HashTable op_table;
	HashTable reg_table;
	HashTable sym_table;

	char in_filename[FILENAME_LEN_MAX];
	char int_filename[FILENAME_LEN_MAX];
	char lst_filename[FILENAME_LEN_MAX];
	char obj_filename[FILENAME_LEN_MAX];
	char prog_name[LABEL_LEN_MAX];
} Assembler;

extern BOOL assemblerInitialize(Assembler* asmblr);
extern void assemblerRelease(Assembler* asmblr);
extern void assemblerAssemble(Assembler* asmblr, const char* filename, FILE* log_stream);
extern void assemblerPrintOpcode(Assembler* asmblr, char* opcode, FILE* stream);
extern void assemblerPrintOpcodeTable(Assembler* asmblr, FILE* stream);
extern void assemblerPrintSymbolTable(Assembler* asmblr, FILE* stream);

#endif