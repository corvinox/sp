#ifndef ASSEBLER_H_
#define ASSEBLER_H_

#include "hash.h"

typedef struct
{
	char label[10];
	char opcode[10];
	char operand[10];
	char comment[10];
	int is_comment;
	int has_label;

} Instruction;




typedef struct
{
	int prog_len;
	int start_addr;
	int locctr;
	HashTable op_table;
} Assembler;

#endif