#include "asmpass2.h"
#include <stdio.h>
#include <stdlib.h>
#include "mytype.h"
#include "assembler.h"
#include "strutil.h"


/* buffer에 관련된 값 정의 */
#define BUFFER_LEN 1024

/* Listing file formatting에 필요한 값을 정의 */
#define LST_LINE_NUMBER_LEN 5
#define LST_LABEL_LEN       6
#define LST_INSTRUCTION_LEN 6
#define LST_OPERAND_LEN     (OBJ_LEN_MAX + 3)

#define TEXT_OBJ_LEN 60

#define MASK_12BITS 0x00000111
#define MASK_24BITS 0x00111111


#define OBJ_LEN_MAX 60

static void statementParse(Assembler* asmblr, Statement* stmt, FILE* stream);

static void lstfileWrite(Statement* stmt, FILE* stream);

static void headerRecordWrite(char* name, int start, int length, FILE* stream);
static void textRecordWrite(int start, char* code, FILE* stream);
static void modificationRecordWrite(int start, int halfs, FILE* stream);
static void endRecordWrite(int excutable, FILE* stream);


BOOL assemblePass2(Assembler* asmblr, FILE* log_stream)
{
	char prog_name[7];
	char text_obj[TEXT_OBJ_LEN + 1] = { 0, };
	Statement stmt = { 0, };
	int base_value = 0;

	FILE* fp_int = fopen(asmblr->int_filename, "r");
	FILE* fp_lst = fopen(asmblr->lst_filename, "w");
	FILE* fp_obj = fopen(asmblr->obj_filename, "w");
	if (fp_int == NULL || fp_lst == NULL || fp_obj == NULL) {
		if (fp_int)
			fclose(fp_int);
		if (fp_lst)
			fclose(fp_lst);
		if (fp_obj)
			fclose(fp_obj);
		return;
	}

	//statementParse(&stmt, buffer, BUFFER_LEN, fp_int);
	if (!strcmp(stmt.instruction->mnemonic, "START")) {
		lstfileWrite(&stmt, fp_lst);
		//statementParse(&stmt, buffer, BUFFER_LEN, fp_int);
	}

	// write header record
	headerRecordWrite(prog_name, 0, 0, fp_obj);
	// init first text record
	while (strcmp(stmt.instruction->mnemonic, "END")) {
		if (!stmt.is_comment) {
			char obj_code[OBJ_LEN_MAX] = { 0, };
			int operand = 0;
			void* opcode = hashGetValue(&asmblr->op_table, stmt.instruction);

			if (opcode != NULL) {
				if (stmt.has_operand) {
					void* sym_value = hashGetValue(&asmblr->sym_table, stmt.operand);
					if (sym_value != NULL) {
						// store symbol value as operand address
						operand = *(int*)sym_value;
					}
					else {
						// store 0 as operand address
						operand = 0;
						// set error flag (undefined symbol)
						asmblr->error = ERR_UNDEFINED_SYMBOL;
					}
				}

				/* TA addressing mode 설정*/
				int pc = stmt.loc + (stmt.is_extended ? 4 : 3);
				int ni;
				int x = (stmt.is_indexed ? 1 : 0);
				int b = 0;
				int p = 0;
				int e = 0;
				/* how to interpret */
				if (stmt.addr_mode == ADDR_SIMPLE)
					ni = 0x11;
				else if (stmt.addr_mode == ADDR_INDIRECT)
					ni = 0x10;
				else if (stmt.addr_mode == ADDR_IMMEDIATE)
					ni = 0x01;

				/* how to caculate */
				int disp = operand - pc;
				if (disp >= -2048 && disp <= 2047) {
					/* PC relative로 계산 */
					/* 음수인 경우 12BIT만 사용하기 위해서 MASKING */
					p = 1;
					disp &= MASK_12BITS;
				}
				else {
					/* PC relative로 불가능하면 Base relative로 계산 */
					disp = operand - base_value;

					if (disp >= 0 && disp <= 4095) {
						b = 1;
					}
					else {
						/* Base relatvie로 불가능하면 format 4 사용 */
						e = 1;
						disp = operand;
					}
				}

				// assemble the object code instruction
				sprintf(obj_code, "%8X%X%X%X%X%*X", *(int*)opcode + ni, x, b, p, e, (e ? 15 : 12), disp);
			}
			else if (!strcmp(stmt.instruction->mnemonic, "BYTE")) {
				int len = strlen(stmt.operand);

				if (stmt.instruction->mnemonic[0] == '\'' && stmt.operand[len - 1] == '\'') {
					if (stmt.operand[0] == 'C')
						asmblr->locctr += len - 3;
					else if (stmt.operand[0] == 'X') {
						if (len & 1) {
							char* endPtr;
							int value = strtol(stmt.operand + 2, &endPtr, 16);
							//if (endPtr - stmt.operand == len - 1)
						}
						else {
							// error
						}
					}
					else {
						// error
					}
				}
				else {
					// error
				}
			}
			else if (!strcmp(stmt.instruction->mnemonic, "WORD")) {
				// convert constant to object code	
				char* endPtr;
				int value = strtol(stmt.operand, &endPtr, 10);
				if (endPtr - stmt.operand == strlen(stmt.operand)) {
					sprintf(obj_code, "%*X", 6, MASK_24BITS & value);
				}
				else {
					// error
				}
			}

			/* object code will not fit into the current Text record */
			if (1) {
				// write text record to objectprogram
				//textRecordWrite(, text_obj, fp_obj);
				// initialize new text record
				memset(text_obj, 0, TEXT_OBJ_LEN);
			}
			// add object code to Text record
		}
		// write lst line
		lstfileWrite(&stmt, fp_lst);
		//statementParse(&stmt, buffer, BUFFER_LEN, fp_int);
	}

	// write last Text record to object program
	//textRecordWrite(, text_obj, fp_obj);
	// write End record to object program
	//endRecordWrite(, fp_obj);
	// write last lst line
	lstfileWrite(&stmt, fp_lst);

	fclose(fp_int);
	fclose(fp_lst);
	fclose(fp_obj);
}

static void statementParse(Assembler* asmblr, Statement* stmt, FILE* stream)
{
	char buffer[BUFFER_LEN] = { 0, };
}

static void lstfileWrite(Statement* stmt, FILE* stream)
{
	if (!stmt->is_comment) {
		fprintf(stream, "%*d    ", LST_LINE_NUMBER_LEN, stmt->line_number);
		fprintf(stream, "%5d    ", stmt->loc);
		fprintf(stream, "%-*s   ", LST_LABEL_LEN, (stmt->has_label ? stmt->label : ""));
		fprintf(stream, "%c", stmt->is_extended ? '+' : ' ');
		fprintf(stream, "%-*s   ", LST_INSTRUCTION_LEN, stmt->instruction->mnemonic);
		if (stmt->addr_mode == ADDR_IMMEDIATE)
			fprintf(stream, "#");
		else if (stmt->addr_mode == ADDR_INDIRECT)
			fprintf(stream, "@");
		else
			fprintf(stream, " ");
		fprintf(stream, "%-*s    ", LST_OPERAND_LEN, (stmt->has_operand ? stmt->operand : ""));
		//fprintf(stream, "")
		fprintf(stream, "\n");
	}
}

static void headerRecordWrite(char* name, int start, int length, FILE* stream)
{
	fprintf(stream, "H%6s%06X%06X\n", name, start, length);
}

static void textRecordWrite(int start, char* code, FILE* stream)
{
	fprintf(stream, "T%6X%2X%s\n", start, strlen(code) / 2, code);
}

static void modificationRecordWrite(int start, int halfs, FILE* stream)
{
	fprintf(stream, "M%6X%2X", start, halfs);
}

static void endRecordWrite(int excutable, FILE* stream)
{
	fprintf(stream, "E%6X", excutable);
}