#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include "strutil.h"

/* buffer에 관련된 값 정의 */
#define BUFFER_LEN 1024
#define BUFFER_SIZE_XXS 8
#define BUFFER_SIZE_XS  16
#define BUFFER_SIZE_S   64
#define BUFFER_SIZE_M   128
#define BUFFER_SIZE_L   256
#define BUFFER_SIZE_XL  1024

#define OBJ_MAX_LEN 60

/* Listing file formatting에 필요한 값을 정의 */
#define LST_LINE_NUMBER_LEN 5
#define LST_LABEL_LEN       6
#define LST_INSTRUCTION_LEN 6
#define LST_OPERAND_LEN     (OBJ_MAX_LEN + 3)

#define TEXT_OBJ_LEN 60

#define MASK_12BITS 0x00000111
#define MASK_24BITS 0x00111111

static int hashFunc(void* key);
static int hashCmp(void* a, void* b);
static void opcodeLoad(Assembler* asmblr);
static void tableRelease(void* data, void* aux);
static void preload(Assembler* asmblr);
static void pass1(Assembler* asmblr, const char* filename);
static void pass2(Assembler* asmblr, const char* filename);
static void parseStatement(Assembler* asmblr, Statement* stmt, FILE* stream);
static int getInstructionCode(Assembler* asmblr, char* str);
static void intfileHeaderWrite(Statement* stmt, FILE* stream);
static void intfileWrite(Statement* stmt, FILE* stream);
static void lstfileWrite(Statement* stmt, FILE* stream);
static void headerRecordWrite(char* name, int start, int length, FILE* stream);
static void textRecordWrite(int start, char* code, FILE* stream);
static void modificationRecordWrite(int start, int halfs, FILE* stream);
static void endRecordWrite(int excutable, FILE* stream);

BOOL assemblerInitialize(Assembler* asmblr)
{
	int idx = 0;
	asmblr->reg_table[idx].mnemonic = "A";
	asmblr->reg_table[idx++].number = 0;
	asmblr->reg_table[idx].mnemonic = "X";
	asmblr->reg_table[idx++].number = 1;
	asmblr->reg_table[idx].mnemonic = "L";
	asmblr->reg_table[idx++].number = 2;
	asmblr->reg_table[idx].mnemonic = "PC";
	asmblr->reg_table[idx++].number = 8;
	asmblr->reg_table[idx].mnemonic = "SW";
	asmblr->reg_table[idx++].number = 9;
	asmblr->reg_table[idx].mnemonic = "B";
	asmblr->reg_table[idx++].number = 3;
	asmblr->reg_table[idx].mnemonic = "S";
	asmblr->reg_table[idx++].number = 4;
	asmblr->reg_table[idx].mnemonic = "T";
	asmblr->reg_table[idx++].number = 5;
	asmblr->reg_table[idx].mnemonic = "F";
	asmblr->reg_table[idx++].number = 6;

	hashInitialize(&asmblr->op_table, hashFunc, hashCmp);
	hashInitialize(&asmblr->sym_table, hashFunc, hashCmp);
	opcodeLoad(asmblr);

	return true;
}

void assemblerAssemble(Assembler* asmblr, const char* filename)
{
	FILE* fp = fopen(filename, "r");
	Statement stmt;
	while(!feof(fp)) {
		parseStatement(asmblr, &stmt, fp);
		if (stmt.is_invalid) {

		}
		else if (stmt.is_comment) {
			printf("comment\n");
		}
		else {
			printf("label: [%s], inst: [%s], operand: [%s]\n", stmt.label, stmt.instruction, stmt.operand);
		}
	}
	fclose(fp);
	return;


	char buffer[256];
	sscanf(filename, "%[^.]", buffer);

	pass1(asmblr, buffer);
	//pass2(asmblr, buffer);
}

void assemblerRelease(Assembler* asmblr)
{
	hashForeach(&asmblr->op_table, NULL, tableRelease);
	hashClear(&asmblr->op_table);
}

/*************************************************************************************
* 설명: 인자로 전달된 key로부터 적절한 hash 값을 얻어낸다.
*       기본으로 제공되는 hash function 이다.
* 인자:
* - key: key
* 반환값: 해당 key를 이용하여 계산한 hash 값
*************************************************************************************/
static int hashFunc(void* key)
{
	char* str = (char*)key;
	int sum = 0;
	int len = strlen(str);
	for (int i = 0; i < len; i++)
		sum += str[i];

	return sum % 20;
}

/*************************************************************************************
* 설명: 임의의 key값을 넣으면 hash table의 entry 중에서 같은 key를 갖고 있는
*       entry를 찾기 위한 비교 함수이다.
* 인자:
* - key0: entry의 key 혹은 사용자가 검색을 원하는 key. 같은지만 비교하므로 순서상관없음.
* - key1: 사용자가 검색을 원하는 key 혹은 entry의 key. 같은지만 비교하므로 순서상관없음.
* 반환값: 같으면 0, 다르면 그 이외의 값
*************************************************************************************/
static int hashCmp(void* key0, void* key1)
{
	char* str_a = (char*)key0;
	char* str_b = (char*)key1;

	return strcmp(str_a, str_b);
}

/*************************************************************************************
* 설명: opcode.txt 파일로부터 opcode에 대한 정보(code, mnemonic, type) 등을
*       읽어서 hash table에 저장한다. 각 정보는 공백으로 구분된다고 가정하고
*       strtok를 이용하여 파일을 줄 단위로 읽어 파싱한다. 읽은 줄에 대해서
*       파싱하는 도중에 예외가 발생하면 해당 줄은 건너뛰고 다음 줄을 읽는다.
* 인자:
* - asmblr: Assembler에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void opcodeLoad(Assembler* asmblr)
{
	char buffer[BUFFER_LEN];
	FILE* fp = fopen("./opcode.txt", "r");
	if (fp == NULL) {
		asmblr->error = ERR_NO_INIT;
		return;
	}

	while (!feof(fp)) {
		fgets(buffer, BUFFER_LEN, fp);
		buffer[strlen(buffer) - 1] = 0;

		/* parse opcode */
		char* ptr = strtok(buffer, " \t");
		if (ptr == NULL)
			continue;
		int code = (int)strtoul(ptr, NULL, 16);

		/* parse mnemonic */
		ptr = strtok(NULL, " \t");
		if (ptr == NULL)
			continue;

		char* mne = strdup(ptr);
		int* code_ptr = (int*)malloc(sizeof(int));
		if (mne != NULL && code_ptr != NULL) {
			*code_ptr = code;
			hashInsert(&asmblr->op_table, mne, code_ptr);
		}
		else {
			if (mne != NULL)
				free(mne);
			if (code_ptr != NULL)
				free(code_ptr);
		}
	}
}

/*************************************************************************************
* 설명: 어셈블러가 가진 opcode table과 symbol table은 hash table로 구성된다.
*       두 테이블의 각 entry의 key, value에 값을 넣을 때 메모리를 할당했으므로,
*       할당한 메모리를 해제해 주어야 한다. hash table의 모든 entry에 적용되는
*       action function으로 data에 할당한 메모리를 해제하는 역할을 한다.
* 인자:
* - data: hash table 각 entry를 나타내는 변수.
* - aux: 추가적으로 필요하면 활용하기 위한 변수. auxiliary
* 반환값: 없음
*************************************************************************************/
static void tableRelease(void* data, void* aux)
{
	if (data != NULL) {
		HashEntry* entry = (HashEntry*)data;
		if (entry->key != NULL)
			free(entry->key);
		if (entry->value != NULL) {
			free(entry->value);
		}
		free(data);
	}
}

static void preload(Assembler* asmblr)
{
	//hashForeach(&asmblr->sym_table, NULL, );
	//hashClear(&asmblr->sym_table);
	//asmblr->reg_table[idx].mnemonic = "A";
	//asmblr->reg_table[idx++].number = 0;
	//asmblr->reg_table[idx].mnemonic = "X";
	//asmblr->reg_table[idx++].number = 1;
	//asmblr->reg_table[idx].mnemonic = "L";
	//asmblr->reg_table[idx++].number = 2;
	//asmblr->reg_table[idx].mnemonic = "PC";
	//asmblr->reg_table[idx++].number = 8;
	//asmblr->reg_table[idx].mnemonic = "SW";
	//asmblr->reg_table[idx++].number = 9;
	//asmblr->reg_table[idx].mnemonic = "B";
	//asmblr->reg_table[idx++].number = 3;
	//asmblr->reg_table[idx].mnemonic = "S";
	//asmblr->reg_table[idx++].number = 4;
	//asmblr->reg_table[idx].mnemonic = "T";
	//asmblr->reg_table[idx++].number = 5;
	//asmblr->reg_table[idx].mnemonic = "F";
	//asmblr->reg_table[idx++].number = 6;

	for (int i = 0; i < REG_CNT; i++) {
		//hashInsert(&asmblr->sym_table, asmblr->reg_table[i].mnemonic, )
	}
}

static void pass1(Assembler* asmblr, const char* filename)
{
	char buffer[BUFFER_LEN];
	Statement stmt;

	sprintf(buffer, "%s.asm", filename);
	FILE* fp_asm = fopen(buffer, "r");

	sprintf(buffer, "%s.int", filename);
	FILE* fp_int = fopen(buffer, "w");
	if (fp_asm == NULL || fp_int == NULL) {
		if (fp_asm)
			fclose(fp_asm);
		if (fp_int)
			fclose(fp_int);
		return;
	}

	stmt.line_number = 0;

	//parseStatement(&stmt, buffer, BUFFER_LEN, fp_asm);
	if (!strcmp(stmt.instruction, "START")) {
		if (!isHexadecimalStr(stmt.operand)) {
			// error
			printf("        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, (stmt.operand == NULL ? "" : stmt.operand));
		}
		// save #[OPERAND] as starting address
		asmblr->start_addr = strtol(stmt.operand, NULL, 16);

		// write line to intermediate file
		//parseStatement(&stmt, buffer, BUFFER_LEN, fp_asm);
	}
	else {
		asmblr->start_addr = 0;
		asmblr->locctr = 0;
	}

	while (strcmp(stmt.instruction, "END")) {
		if (!stmt.is_comment) {
			if (stmt.has_label) {
				// search SYMTAB for LABEL
				void* label = hashGetValue(&asmblr->sym_table, stmt.label);
				if (label != NULL) {
					// error
					asmblr->error = ERR_DUPLICATE_SYMBOL;
					printf("        line %d: 이미 존재하는 SYMBOL (%s)\n", stmt.line_number, (stmt.label == NULL ? "" : stmt.label));
				}
				else {
					char* key = strdup(stmt.label);
					int* value = (int*)malloc(sizeof(int));
					*value = asmblr->locctr;

					/* insert (LABEL, LOCCTR) into SYMTAB */
					hashInsert(&asmblr->sym_table, key, value);
				}
			}

			void* value = hashGetValue(&asmblr->op_table, stmt.instruction);
			if (value != NULL) {
				asmblr->locctr += (stmt.is_extended ? 4 : 3);
			}
			else if (!strcmp(stmt.instruction, "WORD")) {
				asmblr->locctr += 3;
			}
			else if (!strcmp(stmt.instruction, "RESW")) {
				if (isDecimalStr(stmt.operand)) {
					int operand = strtol(stmt.operand, NULL, 10);
					asmblr->locctr += 3 * operand;
				}
				else {
					// error
					printf("        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, (stmt.operand == NULL ? "" : stmt.operand));
				}
			}
			else if (!strcmp(stmt.instruction, "RESB")) {
				if (isDecimalStr(stmt.operand)) {
					int operand = strtol(stmt.operand, NULL, 10);
					asmblr->locctr += operand;
				}
				else {
					// error
					printf("        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, (stmt.operand == NULL ? "" : stmt.operand));
				}
			}
			else if (!strcmp(stmt.instruction, "BYTE")) {
				int len = strlen(stmt.operand);

				if (stmt.operand[0] == '\'' && stmt.operand[len - 1] == '\'') {
					if (stmt.operand[0] == 'C')
						asmblr->locctr += len - 3;
					else if (stmt.operand[0] == 'X')
						asmblr->locctr += (len - 2) / 2;
					else {
						// error
						printf("        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, (stmt.operand == NULL ? "" : stmt.operand));
					}
				}
				else {
					// error
					printf("        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, (stmt.operand == NULL ? "" : stmt.operand));
				}
			}
		}

		// write int
		//parseStatement(&stmt, buffer, BUFFER_LEN, fp_asm);
	}
	// write int
	asmblr->prog_len = asmblr->locctr - asmblr->start_addr;

	fclose(fp_asm);
	fclose(fp_int);
}

static void pass2(Assembler* asmblr, const char* filename)
{
	char prog_name[7];
	char buffer[BUFFER_LEN];
	char text_obj[TEXT_OBJ_LEN + 1] = { 0, };
	Statement stmt;
	int base_value = 0;

	sprintf(buffer, "%s.int", filename);
	FILE* fp_int = fopen(buffer, "r");

	sprintf(buffer, "%s.lst", filename);
	FILE* fp_lst = fopen(filename, "w");

	sprintf(buffer, "%s.obj", filename);
	FILE* fp_obj = fopen(filename, "w");
	if (fp_int == NULL || fp_lst == NULL || fp_obj == NULL) {
		if (fp_int)
			fclose(fp_int);
		if (fp_lst)
			fclose(fp_lst);
		if (fp_obj)
			fclose(fp_obj);
		return;
	}

	//parseStatement(&stmt, buffer, BUFFER_LEN, fp_int);
	if (!strcmp(stmt.instruction, "START")) {
		lstfileWrite(&stmt, fp_lst);
		//parseStatement(&stmt, buffer, BUFFER_LEN, fp_int);
	}

	// write header record
	headerRecordWrite(prog_name, 0, 0, fp_obj);
	// init first text record
	while (strcmp(stmt.instruction, "END")) {
		if (!stmt.is_comment) {
			char obj_code[OBJ_MAX_LEN] = { 0, };
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
			else if (!strcmp(stmt.instruction, "BYTE")) {
				int len = strlen(stmt.operand);

				if (stmt.instruction[0] == '\'' && stmt.operand[len - 1] == '\'') {
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
			else if (!strcmp(stmt.instruction, "WORD")) {
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
		//parseStatement(&stmt, buffer, BUFFER_LEN, fp_int);
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

static void parseStatement(Assembler* asmblr, Statement* stmt, FILE* stream) 
{
	char* begin;
	char* end;
	char buffer[BUFFER_LEN];
	char buffer2[BUFFER_LEN];

	if (fgets(buffer, BUFFER_LEN, stream) == NULL) {
		/* 파일의 끝이면 현재 statement가 invalid 한 것으로 설정 */
		stmt->is_invalid = true;
		return;
	}

	/* 앞,뒤 공백을 제거 */
	begin = strTrim(buffer, buffer + strlen(buffer));

	if (strlen(begin) == 0) {
		/* 빈 문자열인 경우 invalid한 statement */
		stmt->is_invalid = true;
	}
	else if (begin[0] == '.') {
		/* comment인 경우 */
		stmt->is_comment = true;
	}
	else {		
		/* 위의 경우가 모두 아닌 경우 comment도 아니고 valid 하다고 판단 */
		stmt->is_comment = false;
		stmt->is_invalid = false;

		stmt->line_number += 5;

		strParse(begin, &begin, &end);
		strncpy(buffer2, begin, end - begin);
		buffer2[end - begin] = 0;

		/* 파싱한 첫 문자열이 instruction이 아니면 label로 인식 */
		int inst_code = getInstructionCode(asmblr, buffer2);
		if (inst_code == INST_NO) {
			/* label 설정 */
			stmt->has_label = true;
			strcpy(stmt->label, buffer2);

			strParse(end, &begin, &end);
			strncpy(buffer2, begin, end - begin);
			buffer2[end - begin] = 0;
			/* label 뒤에 파싱한 문자열이 instruction이 아니면 에러 */
			inst_code = getInstructionCode(asmblr, buffer2);
			if (inst_code == INST_NO) {
				/* error: instruction 없음 */
			}
			else if (strlen(buffer2) > INSTRUCTION_LEN) {
				/* error: 너무 긴 instruction */
			} else {
				/* instruction 설정 */
				stmt->inst_code = inst_code;
				strcpy(stmt->instruction, buffer2);
			}
		}
		else if (strlen(buffer2) > INSTRUCTION_LEN) {
			/* error: 너무 긴 instruction */
		}
		else {
			/* label 없음 설정*/
			stmt->has_label = false;
			stmt->label[0] = 0;

			/* instruction 설정 */
			stmt->inst_code = inst_code;
			strcpy(stmt->instruction, buffer2);
		}

		/* operand 설정 */
		begin = strTrimFront(end);
		int operand_len = strlen(begin);
		if (operand_len > OPERAND_LEN) {
			/* error: 너무 긴 operand */
		}
		else if (operand_len <= 0) {
			/* operand 없음 설정 */
			stmt->has_operand = false;
			stmt->operand[0] = 0;
		}
		else {
			/* operand 설정 */
			stmt->has_operand = true;
			strcpy(stmt->operand, begin);
		}
	}
}

static int getInstructionCode(Assembler* asmblr, char* str)
{
	if (!strcmp(str, "START")) {
		return INST_START;
	}
	else if (!strcmp(str, "END")) {
		return INST_END;
	}
	else if (!strcmp(str, "BYTE")) {
		return INST_BYTE;
	}
	else if (!strcmp(str, "WORD")) {
		return INST_WORD;
	}
	else if (!strcmp(str, "RESB")) {
		return INST_RESB;
	}
	else if (!strcmp(str, "RESW")) {
		return INST_RESW;
	}
	else if (!strcmp(str, "BASE")) {
		return INST_BASE;
	}
	else {
		void* value = NULL;
		if (str[0] == '+')
			value = hashGetValue(&asmblr->op_table, str + 1);
		else
			value = hashGetValue(&asmblr->op_table, str);

		if (value != NULL) {
			return INST_OPCODE;
		}
	}

	return INST_NO;
}

static void intfileHeaderWrite(Statement* stmt, FILE* stream)
{
	fprintf(stream, "%d", stmt->line_number);
}

static void intfileWrite(Statement* stmt, FILE* stream)
{
	fprintf(stream, "%d", stmt->line_number);
}

static void lstfileWrite(Statement* stmt, FILE* stream)
{
	if (!stmt->is_comment) {
		fprintf(stream, "%*d    ", LST_LINE_NUMBER_LEN, stmt->line_number);
		fprintf(stream, "%5d    ", stmt->loc);
		fprintf(stream, "%-*s   ", LST_LABEL_LEN, (stmt->has_label ? stmt->label : ""));
		fprintf(stream, "%c", stmt->is_extended ? '+' : ' ');
		fprintf(stream, "%-*s   ", LST_INSTRUCTION_LEN, stmt->instruction);
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