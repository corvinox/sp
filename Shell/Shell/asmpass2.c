#include "asmpass2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "mytype.h"
#include "assembler.h"
#include "asminstruction.h"
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
static void headerRecordWrite(HeaderRecord* record, FILE* stream);
static void textRecordWrite(TextRecord* record, FILE* stream);
static void modificationRecordWrite(EndRecord* record, FILE* stream);
static void endRecordWrite(EndRecord* record, FILE* stream);

BOOL assemblePass2(Assembler* asmblr, FILE* log_stream)
{
	/* assembler pass2 진행 상태로 전환 */
	asmblr->state = STAT_RUN_PASS2;

	FILE* fp_int = fopen(asmblr->int_filename, "r");
	FILE* fp_lst = fopen(asmblr->lst_filename, "w");
	FILE* fp_obj = fopen(asmblr->obj_filename, "w");
	if (fp_int == NULL || fp_lst == NULL || fp_obj == NULL) {
		if (fp_int)
			fclose(fp_int);
		else
			fprintf(log_stream, "%s 파일을 열 수 없습니다.\n", asmblr->int_filename);
		if (fp_lst)
			fclose(fp_lst);
		else
			fprintf(log_stream, "%s 파일을 열 수 없습니다.\n", asmblr->lst_filename);
		if (fp_obj)
			fclose(fp_obj);
		else
			fprintf(log_stream, "%s 파일을 열 수 없습니다.\n", asmblr->obj_filename);
		return false;
	}

	Statement stmt = { 0, };
	HeaderRecord h_record = { 0, };
	TextRecord t_record = { 0, };
	EndRecord e_record = { 0, };

	BOOL pass_error = false;

	statementParse(asmblr, &stmt, fp_int);
	if (stmt.is_invalid || stmt.instruction == NULL) {

	}
	else if (stmt.instruction->type == INST_START) {
		strcpy(h_record.prog_name, stmt.label);
		lstfileWrite(&stmt, fp_lst);
		statementParse(asmblr, &stmt, fp_int);
	}

	/* write header record */
	headerRecordWrite(&h_record, fp_obj);
	/* init first text record */


	while (stmt.instruction != NULL && stmt.instruction->type == INST_END) {
		if (stmt.is_invalid) {
		}
		else if (stmt.is_empty) {
		}
		else if (!stmt.is_comment) {
			char obj_code[OBJ_LEN_MAX + 1] = { 0, };
			int operand = 0;

			//asmblr->pc_value += stmt.
			
			if (stmt.instruction->type == INST_OPCODE) {
				int opcode_format = (int)stmt.instruction->aux;
				if (opcode_format != 1 && !stmt.has_operand) {
					/* error */
					stmt.error = true;
					fprintf(log_stream, "LINE %d: INSTRUCTION (%s) 는 OPERAND가 필요합니다.\n", stmt.line_number, stmt.instruction->mnemonic);
				}
				else if (opcode_format != 3 && stmt.is_extended) {
					/* error */
					stmt.error = true;
					fprintf(log_stream, "LINE %d: INSTRUCTION (%s) FORMAT4로 사용 할 수 없습니다.\n", stmt.line_number, stmt.instruction->mnemonic);
				}
				else if (opcode_format == 2) {

				}
				else if (opcode_format == 3) {
					char buffer[BUFFER_LEN] = { 0, };
					char* begin = stmt.operand;
					char* end;
					int bit_n = 0, bit_i = 0;
					int bit_b = 0, bit_p = 0;
					int bit_x = 0, bit_e = 0;
					int addr = 0;

					if (*begin == '@') {
						begin++;
					}
					else if (*begin == '#') {
						begin++;
					}

					strcpy(buffer, begin);
					char* ctx;
					char* ptr = strtok_s(begin, ",", &ctx);
					
					if (ptr == NULL) {
						stmt.error = true;
					}
					else {
						ptr = strTrim(ptr, ctx);

						int* value = (int*)hashGetValue(&asmblr->sym_table, ptr);
						if (value == NULL) {
							stmt.error = true;
							fprintf(log_stream, "LINE %d: SYMBOL (%s)이 정의되지 않았습니다.\n", stmt.line_number, stmt.operand);
						}

						/* 해당 symbol에 대한 주소값 저장 */
						addr = *value;
						if ((ptr = strtok_s(ctx, ",", &ctx)) != NULL) {
							ptr = strTrim(ptr, ctx);
							if (strcmp(ptr, "X")) {
								/* error */
								stmt.error = true;
								fprintf(log_stream, "LINE %d: 잘못된 문법입니다.\n", stmt.line_number);
							}
							else {
								/* indexed addressing 모드 */
								bit_x = 1;
							}
						}

						if (stmt.is_extended) {
							bit_e = 1;
						}
						else {
							/* pc relative */
							int disp = addr - stmt.loc - 3;
							if (disp >= -2048 && disp <= 2047) {
							}
						}
					}
				}
				if (stmt.has_operand) {
					void* sym_value = hashGetValue(&asmblr->sym_table, stmt.operand);
					if (sym_value != NULL) {
						/* 해당 심볼에 대한 location counter 값 저장 */
						operand = *(int*)sym_value;
					}
					else {
						/* error */
						stmt.error = true;
						fprintf(log_stream, "LINE %d: SYMBOL (%s)이 정의되지 않았습니다.\n", stmt.line_number, stmt.operand);
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
					disp = operand - asmblr->pc_value;

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
				//sprintf(obj_code, "%8X%X%X%X%X%*X", *(int*)opcode + ni, x, b, p, e, (e ? 15 : 12), disp);
			}
			else if (!strcmp(stmt.instruction->mnemonic, "BYTE")) {
				int len = strlen(stmt.operand);

				if (stmt.instruction->mnemonic[0] == '\'' && stmt.operand[len - 1] == '\'') {
					if (stmt.operand[0] == 'C')
						asmblr->pc_value += len - 3;
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
			//if (1) {
				// write text record to objectprogram
				//textRecordWrite(, text_obj, fp_obj);
				// initialize new text record
				//memset(text_obj, 0, TEXT_OBJ_LEN);
			//}
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

	return !pass_error;
}

static void statementParse(Assembler* asmblr, Statement* stmt, FILE* stream)
{
	char buffer[BUFFER_LEN] = { 0, };
	char* ptr;
	
	/* 문자열 4줄이 한 묶음. 4줄이 전부 입력되지 않고, 파일이 끝나면 에러 statement로 간주 */

	/* 1. line number, location counter */
	if (fgets(buffer, BUFFER_LEN, stream) == NULL) {
		/* 파일의 끝이면 현재 statement가 빈 문자열인 것으로 설정 */
		return;
	}
	else {
		sscanf(buffer, "%d %d", &stmt->line_number, &stmt->loc);
	}
	
	/* 2. label */
	if (fgets(buffer, BUFFER_LEN, stream) == NULL) {
		/* 파일의 끝이면 현재 statement가 빈 문자열인 것으로 설정 */
		stmt->is_invalid = true;
		return;
	}
	else {
		ptr = strTrim(buffer, buffer + strlen(buffer));
		if (strlen(ptr) > 0) {
			sscanf(ptr, "%s", &stmt->label);
			stmt->has_label = true;
		}
	}
	
	/* 3. instruction */
	if (fgets(buffer, BUFFER_LEN, stream) == NULL) {
		/* 파일의 끝이면 현재 statement가 빈 문자열인 것으로 설정 */
		stmt->is_invalid = true;
		return;
	}
	else {
		ptr = strTrim(buffer, buffer + strlen(buffer));
		stmt->instruction = getInstruction(asmblr, ptr);
		if (stmt->instruction == NULL) {
			/* error */
			stmt->is_invalid = true;
			return;
		}
	}	

	/* 4. operand */
	if (fgets(buffer, BUFFER_LEN, stream) == NULL) {
		/* 파일의 끝이면 현재 statement가 빈 문자열인 것으로 설정 */
		stmt->is_invalid = true;
		return;
	}
	else {
		ptr = strTrim(buffer, buffer + strlen(buffer));
		if (strlen(ptr) > 0) {
			sscanf(ptr, "%s", &stmt->operand);
			stmt->has_operand = true;
		}
	}
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

static void headerRecordWrite(HeaderRecord* record, FILE* stream)
{
	fprintf(stream, "H%6s%06X%06X\n", record->prog_name, record->prog_addr, record->prog_len);
}

static void textRecordWrite(TextRecord* record, FILE* stream)
{
	fprintf(stream, "T%6X%2X%s\n", record->obj_addr, record->obj_len, record->obj_code);
}

static void modificationRecordWrite(EndRecord* record, FILE* stream)
{
	fprintf(stream, "M%6X%2X", record->executable, record->executable);
}

static void endRecordWrite(EndRecord* record, FILE* stream)
{
	fprintf(stream, "E%6X", record->executable);
}