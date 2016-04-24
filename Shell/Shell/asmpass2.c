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
#define LST_GAP_LEN         4
#define LST_LINE_NUMBER_LEN 5
#define LST_LABEL_LEN       6
#define LST_INSTRUCTION_LEN 6
#define LST_OPERAND_LEN     (OBJ_LEN_MAX + 3)


#define TEXT_OBJ_LEN 60

#define MASK_1BIT   0x00000001
#define MASK_4BITS  0x0000000F
#define MASK_12BITS 0x00000FFF
#define MASK_16BITS 0x0000FFFF
#define MASK_20BITS 0x000FFFFF
#define MASK_24BITS 0x00FFFFFF

static void statementParse(Assembler* asmblr, Statement* stmt, FILE* stream);
static void lstfileWrite(Statement* stmt, char* obj_code, FILE* stream);
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
	char buffer[BUFFER_LEN] = { 0, };

	/* assembler 변수 초기화 */
	asmblr->pc_value = 0;

	statementParse(asmblr, &stmt, fp_int);
	if (stmt.is_invalid || stmt.instruction == NULL) {

	}
	else if (stmt.instruction->type == INST_START) {
		strcpy(h_record.prog_name, stmt.label);
		lstfileWrite(&stmt, "", fp_lst);
		statementParse(asmblr, &stmt, fp_int);
	}

	/* write header record */
	h_record.prog_addr = asmblr->prog_addr;
	h_record.prog_len = asmblr->prog_len;
	strcpy(h_record.prog_name, asmblr->prog_name);
	headerRecordWrite(&h_record, fp_obj);

	/* init first text record */
	t_record.obj_addr = asmblr->prog_addr;

	while (!feof(fp_int)) {
		if (stmt.is_invalid) {
			/* error */
			stmt.error = true;
			fprintf(log_stream, "LINE %d: 유효하지 않은 STATEMENT 입니다.\n", stmt.line_number);
		}
		else if (stmt.is_empty) {
		}
		else if (!stmt.is_comment) {
			char obj_code[OBJ_LEN_MAX + 1] = { 0, };

			asmblr->pc_value += stmt.inst_len;
			
			if (stmt.instruction->type == INST_END) {

				/* object code 파일에 마지막 text record 쓰기 */
				textRecordWrite(&t_record, fp_obj);

				/* object code 파일에 end record 쓰기 */
				endRecordWrite(&e_record, fp_obj);

				// write last lst line
				lstfileWrite(&stmt, "", fp_lst);
				break;
			}
			else if (stmt.instruction->type == INST_BASE) {
				int* value = (int*)hashGetValue(&asmblr->sym_table, stmt.operand);
				if (value == NULL) {
					BOOL error;
					int base_value = strToInt(stmt.operand, 10, &error);
					if (error) {
						/* error */
						stmt.error = true;
						fprintf(log_stream, "LINE %d: OPERAND (%s)는 유효하지 않습니다.\n", stmt.line_number, stmt.operand);
					}
					else {
						asmblr->base_value = base_value;
					}
				}
				else {
					asmblr->base_value = *value;
				}
			}
			else if (stmt.instruction->type == INST_OPCODE) {
				int opcode_format = (int)stmt.instruction->aux;
				if (opcode_format != 3 && stmt.is_extended) {
					/* error */
					stmt.error = true;
					fprintf(log_stream, "LINE %d: INSTRUCTION (%s)는 FORMAT4로 사용 될 수 없습니다.\n", stmt.line_number, stmt.instruction->mnemonic);
				}
				/* opcode instruction의 format이 1 인 경우 */
				else if (opcode_format == 1) {
					sprintf(obj_code, "%8X", (BYTE)stmt.instruction->value);
				}
				/* opcode instruction의 format이 2 인 경우 */
				else if (opcode_format == 2) {
					AsmRegister* reg;
					int r0 = 0;
					int r1 = 0;

					strcpy(buffer, stmt.operand);
					char* ctx;
					char* ptr = strtok_s(buffer, ",", &ctx);
					if (ptr == NULL) {
						/* error */
						stmt.error = true;
						fprintf(log_stream, "LINE %d: INSTRUCTION (%s) 는 OPERAND가 필요합니다.\n", stmt.line_number, stmt.instruction->mnemonic);
					}
					else {
						ptr = strTrim(ptr, ctx - 1);
						reg = (AsmRegister*)hashGetValue(&asmblr->reg_table, ptr);
						if (reg == NULL) {
							/* error */
							stmt.error = true;
							fprintf(log_stream, "LINE %d: 해당 이름의 REGISTER (%s) 를 찾을 수 없습니다.\n", stmt.line_number, ptr);
						}
						else {
							r0 = reg->number;
							
							ptr = strtok_s(ctx, ",", &ctx);
							if (ptr != NULL) {
								ptr = strTrim(ptr, ctx - 1);
								if (strlen(ptr) > 0) {
									reg = (AsmRegister*)hashGetValue(&asmblr->reg_table, ptr);
									if (reg == NULL) {
										/* error */
										stmt.error = true;
										fprintf(log_stream, "LINE %d: 해당 이름의 REGISTER (%s) 를 찾을 수 없습니다.\n", stmt.line_number, ptr);
									}
									else {
										r1 = reg->number;
									}
								}
							}

							WORD obj_value = (BYTE)stmt.instruction->value;
							obj_value = (obj_value << 4) + (MASK_4BITS & r0);
							obj_value = (obj_value << 4) + (MASK_4BITS & r1);
							sprintf(obj_code, "%024X", MASK_16BITS & obj_value);
						}
					}
				}
				/* opcode instruction의 format이 3/4 인 경우 */
				else if (opcode_format == 3) {
					BYTE bit_n = 1, bit_i = 1;
					BYTE bit_b = 0, bit_p = 0;
					BYTE bit_x = 0, bit_e = 0;
					int addr = 0;

					if (stmt.has_operand) {

					}
					else {
						sprintf(obj_code, "%8X", (BYTE)stmt.instruction->value);
					}

					if (!stmt.error) {

					}

					char* begin = stmt.operand;
					
					/* indirect mode 인지 검사 */
					if (*begin == '@') {
						bit_i = 0;
						begin++;
					}
					/* immediate mode 인지 검사 */
					else if (*begin == '#') {
						bit_n = 0;
						begin++;
					}

					strcpy(buffer, begin);
					char* ctx;
					char* ptr = strtok_s(begin, ",", &ctx);
					
					if (ptr == NULL) {
						/* error */
						stmt.error = true;
						fprintf(log_stream, "LINE %d: INSTRUCTION (%s) 는 OPERAND가 필요합니다.\n", stmt.line_number, stmt.instruction->mnemonic);
					}
					else {
						ptr = strTrim(ptr, ctx - 1);

						int* value = (int*)hashGetValue(&asmblr->sym_table, ptr);
						if (value == NULL) {
							BOOL error;
							addr = strToInt(ptr, 10, &error);
							if (error) {
								/* error */
								stmt.error = true;
								fprintf(log_stream, "LINE %d: OPERAND (%s)는 유효하지 않습니다.\n", stmt.line_number, stmt.operand);
							}
						}
						else {
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

							/* format 4 사용을 명시한 경우 */
							if (stmt.is_extended) {
								bit_e = 1;

								WORD obj_value = (BYTE)stmt.instruction->value;
								obj_value += (bit_n << 1) + bit_i;
								obj_value = (obj_value << 1) + MASK_1BIT & bit_x;
								obj_value <<= 1, // bit_b = 0
									obj_value <<= 1; // bit_p = 0;
								obj_value = (obj_value << 1) + 1;
								obj_value = (obj_value << 12) + MASK_20BITS & addr;
								sprintf(obj_code, "%032X", obj_value);
							}
							/* format 3인 경우 */
							else {
								/* pc relative 시도 */
								int disp = addr - asmblr->pc_value;
								if (disp >= -2048 && disp <= 2047) {
									bit_p = 1;
								}
								/* pc relative가 안되면 base relative 시도 */
								else {
									disp = addr - asmblr->base_value;
									if (disp >= 0 && disp <= 4095) {
										bit_b = 1;
									}
									/* base relatvie로도 주소 계산이 불가능하면 에러 */
									/* pc, base  둘다 불가능하여 format 4를 사용해야하는데 */
									/* format 4를 사용을 명시하는 prefix '+'를 사용하지 않았기 때문에 에러 */
									else {
										stmt.error = true;
										fprintf(log_stream, "LINE %d: FORMAT 4 사용이 명시되어있지 않습니다. \n", stmt.line_number);
									}
								}

								/* assemble the object code instruction */
								WORD obj_value = (BYTE)stmt.instruction->value;
								obj_value += (bit_n << 1) + bit_i;
								obj_value = (obj_value << 1) + (MASK_1BIT & bit_x);
								obj_value = (obj_value << 1) + (MASK_1BIT & bit_b);
								obj_value = (obj_value << 1) + (MASK_1BIT & bit_p);
								obj_value <<= 1; // bit_e = 0
								obj_value = (obj_value << 12) + (MASK_12BITS & disp);
								sprintf(obj_code, "%024X", MASK_24BITS & obj_value);
							}
						}
					}
				}
			} //
			else if (!strcmp(stmt.instruction->mnemonic, "BYTE")) {
				int len = strlen(stmt.operand);

				if (stmt.operand[1] == '\'' && stmt.operand[len - 1] == '\'') {
					if (stmt.operand[0] == 'C') {
						stmt.inst_len = len - 3;
					}
					else if (stmt.operand[0] == 'X') {
						stmt.inst_len = (len - 2) / 2;
					}
					else {
						/* error */
						stmt.error = true;
						fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, stmt.operand);
					}
				}
				else {
					/* error */
					stmt.error = true;
					fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, stmt.operand);
				}
			}
			else if (!strcmp(stmt.instruction->mnemonic, "WORD")) {
				// convert constant to object code	
				BOOL error;
				int value = strToInt(stmt.operand, 10, &error);
				if (error) {
					/* error */
					stmt.error = true;
					fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, stmt.operand);
				}
				else {
					sprintf(obj_code, "%6X", MASK_24BITS & value);
				}
			}

			int obj_code_len = strlen(obj_code);
			if (obj_code_len & 1) {
				/* error */
				stmt.error = true;
				fprintf(log_stream, "LINE %d: 유효하지 않은 OBJECT CODE가 생성되었습니다.\n", stmt.line_number);
			}

			if (!stmt.error) {
				/* object code will not fit into the current Text record */
				int cal_len = t_record.obj_len + (obj_code_len / 2);
				if (cal_len >= 30) {
					t_record.obj_len = cal_len; 
					strcat(t_record.obj_code, obj_code);

					/* object code 파일에 text record 쓰기 */
					textRecordWrite(&t_record, fp_obj);

					t_record.obj_addr = asmblr->pc_value;
					t_record.obj_len = 0;
					t_record.obj_code[0] = 0;
				}

				/* listing 파일에 쓰기 */
				lstfileWrite(&stmt, obj_code, fp_lst);
			}
			
		} // 
		
		/* statement 초기화 */
		memset(&stmt, 0, sizeof(Statement));
		statementParse(asmblr, &stmt, fp_int);
	}

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
		stmt->is_empty = true;
		return;
	}
	else {
		sscanf(buffer, "%d %d %d", &stmt->line_number, &stmt->loc, &stmt->inst_len);
	}
	
	/* 2. label */
	if (fgets(buffer, BUFFER_LEN, stream) == NULL) {
		/* error */
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
		/* error */
		stmt->is_invalid = true;
		return;
	}
	else {
		ptr = strTrim(buffer, buffer + strlen(buffer));
		if (*ptr == '+') {
			stmt->is_extended = true;
			ptr++;
		}

		stmt->instruction = getInstruction(asmblr, ptr);
		if (stmt->instruction == NULL) {
			/* error */
			stmt->is_invalid = true;
			return;
		}
	}	

	/* 4. operand */
	if (fgets(buffer, BUFFER_LEN, stream) == NULL) {
		/* error */
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

static void lstfileWrite(Statement* stmt, char* obj_code, FILE* stream)
{
	char buffer[BUFFER_LEN + 1] = { 0, };
	char* ptr = buffer;

	sprintf(ptr, "%*d", LST_LINE_NUMBER_LEN, stmt->line_number);
	ptr += LST_LINE_NUMBER_LEN;
	for (int i = 0; i < LST_GAP_LEN; i++)
		sprintf(ptr++, " ");

	sprintf(ptr, "%5d", stmt->loc);
	ptr += 5;
	for (int i = 0; i < LST_GAP_LEN; i++)
		sprintf(ptr++, " ");

	sprintf(ptr, "%-*s", LST_LABEL_LEN, (stmt->has_label ? stmt->label : ""));
	ptr += LST_LABEL_LEN;
	for (int i = 0; i < LST_GAP_LEN; i++)
		sprintf(ptr++, " ");

	sprintf(ptr, "%c", stmt->is_extended ? '+' : ' ');
	ptr += 1;

	sprintf(ptr, "%-*s", LST_INSTRUCTION_LEN, stmt->instruction->mnemonic);
	ptr += LST_INSTRUCTION_LEN;
	for (int i = 0; i < LST_GAP_LEN; i++)
		sprintf(ptr++, " ");

	if (stmt->operand == ADDR_IMMEDIATE)
		sprintf(ptr, "#");
	else if (stmt->addr_mode == ADDR_INDIRECT)
		sprintf(ptr, "@");
	else
		sprintf(ptr, " ");
	ptr += 1;

	sprintf(ptr, "%-*s", LST_OPERAND_LEN, (stmt->has_operand ? stmt->operand : ""));
	ptr += LST_OPERAND_LEN;
	for (int i = 0; i < LST_GAP_LEN; i++)
		sprintf(ptr++, " ");

	sprintf(ptr, "%s\n", obj_code);

	fprintf(stream, "%s", buffer);
}

static void headerRecordWrite(HeaderRecord* record, FILE* stream)
{
	fprintf(stream, "H%6s%06X%06X\n", record->prog_name, record->prog_addr, record->prog_len);
}

static void textRecordWrite(TextRecord* record, FILE* stream)
{
	fprintf(stream, "T%06X%02X%s\n", record->obj_addr, record->obj_len, record->obj_code);
}

static void modificationRecordWrite(EndRecord* record, FILE* stream)
{
	fprintf(stream, "M%6X%2X", record->executable, record->executable);
}

static void endRecordWrite(EndRecord* record, FILE* stream)
{
	fprintf(stream, "E%06X", record->executable);
}