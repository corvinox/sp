#include "assembler.h"
#include <stdio.h>
#include <string.h>
#include "strutil.h"

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#define BUFFER_MAX 1024

void assemble(Assembler* asm) {


}

static void pass1(Assembler* asm, const char* filename)
{
	char buffer[BUFFER_MAX];
	Instruction inst;

	fgets(buffer, BUFFER_MAX, filename);
	// parsing
	if (!strcmp(inst.opcode, "START")){

	}
	else {
		asm->start_addr = 0;
		asm->locctr = 0;
	}

	while (strcmp(inst.opcode, "END")) {
		if (!inst.is_comment) {
			if (inst.has_label) {
				// search SYMTAB for LABEL
				int find = 1;
				if (find) {
					// error
				}
				else {
					// insert LABAL, locctr to SYMTAB
				}
			}

			int value = getValue(&asm->op_table, inst.opcode);
			if (value != NULL) {
				asm->locctr += 3;
			}
			else if(!strcmp(inst.opcode, "WORD")) {
				asm->locctr += 3;
			}
			else if (!strcmp(inst.opcode, "RESW")) {
				//asm->locctr += 3 * #operand;
			}
			else if (!strcmp(inst.opcode, "RESB")) {
				//asm->locctr += #operand;
			}
			else if (!strcmp(inst.opcode, "BYTE")) {
				// asm->locctr += length
			}
		}

		// write int
		fgets(buffer, BUFFER_MAX, filename);
		// parsing
	}
	// write int
	asm->prog_len = asm->locctr - asm->start_addr;
}

static void pass2(Assembler* asm, const char* filename)
{

}

static void parseInstruction(Instruction* inst, const char* buffer) {
	char *str;
	char* ptr;

	if (buffer == NULL) {
		return;
	}

	str = trimAndDup(buffer, buffer + strlen(buffer));
	if (str == NULL) {
		return;
	}

	/* comment인지 검사 */
	if (str[0] == '.') {
		inst->is_comment = true;
		return;
	}
	
	/*  */
	ptr = strtok(str, " \t\n");
	if (ptr == NULL) {
		return;
	}

	

	free(str);
}