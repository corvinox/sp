#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strutil.h"

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#define BUFFER_MAX 1024

static void pass1(Assembler* asm, const char* filename);
static void pass2(Assembler* asm, const char* filename); 
static void parseStatement(Statement* stmt, const char* buffer);

void initializeAssembler(Assembler* asm)
{

}

void assemble(Assembler* asm, const char* filename)
{
	pass1(asm, filename);
}

static void pass1(Assembler* asm, const char* filename)
{
	char buffer[BUFFER_MAX];
	Statement stmt;
	FILE* fp_asm = fopen(filename, "r");
	//FILE* fp_lst = fopen(filename, "w");
	//if (fp_asm == NULL || fp_lst == NULL) {
	//	return;
	//}

	fgets(buffer, BUFFER_MAX, fp_asm);
	parseStatement(&stmt, buffer);
	if (!strcmp(stmt.opcode, "START")){

	}
	else {
		asm->start_addr = 0;
		asm->locctr = 0;
	}

	while (strcmp(stmt.opcode, "END")) {
		if (!stmt.is_comment) {
			if (stmt.has_label) {
				// search SYMTAB for LABEL
				int find = 1;
				if (find) {
					// error
				}
				else {
					// insert LABAL, locctr to SYMTAB
				}
			}

			void* value = getValue(&asm->op_table, stmt.opcode);
			if (value != NULL) {
				asm->locctr += 3;
			}
			else if(!strcmp(stmt.opcode, "WORD")) {
				asm->locctr += 3;
			}
			else if (!strcmp(stmt.opcode, "RESW")) {
				//asm->locctr += 3 * #operand;
			}
			else if (!strcmp(stmt.opcode, "RESB")) {
				//asm->locctr += #operand;
			}
			else if (!strcmp(stmt.opcode, "BYTE")) {
				// asm->locctr += length
			}
		}

		// write int
		fgets(buffer, BUFFER_MAX, fp_asm);
		// parsing
	}
	// write int
	asm->prog_len = asm->locctr - asm->start_addr;

	fclose(fp_asm);
	//fclose(fp_lst);
}

static void pass2(Assembler* asm, const char* filename)
{

}

static void parseStatement(Statement* stmt, const char* buffer) {
	char* str;
	char* ptr;
	char* savePtr;

	if (buffer == NULL) {
		return;
	}

	str = strTrimDup(buffer, buffer + strlen(buffer));
	if (str == NULL) {
		return;
	}

	/* comment인지 검사 */
	if (str[0] == '.') {
		stmt->is_comment = true;
		return;
	}
	
	/*  */
	ptr = strParseDup(str, &savePtr);
	if (ptr == NULL) {
		return;
	}

	ptr = strParseDup(NULL, &savePtr);
	if (ptr == NULL) {
		return;
	}

	ptr = strParseDup(NULL, &savePtr);
	if (ptr == NULL) {
		return;
	}

	free(str);
}