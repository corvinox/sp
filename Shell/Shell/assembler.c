#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include "strutil.h"

#define BUFFER_MAX 1024

#define OBJ_MAX_LEN 30

#define LST_LINE_NUMBER_LEN 5
#define LST_LABEL_LEN       6
#define LST_INSTRUCTION_LEN 6
#define LST_OPERAND_LEN     (OBJ_MAX_LEN + 3)

#define TEXT_OBJ_LEN 60

static int hashFunc(void* key);
static int hashCmp(void* a, void* b);
static void opcodeLoad(Assembler* asmblr);
static void tableRelease(void* data, void* aux);
static void preload(Assembler* asmblr);
static void pass1(Assembler* asmblr, const char* filename);
static void pass2(Assembler* asmblr, const char* filename);
static void parseStatement(Statement* stmt, char* buffer, int maxCount, FILE* stream);
static void lstfileWrite(Statement* stmt, FILE* stream);
static void headerRecordWrite(char* name, int start, int length, FILE* stream);
static void textRecordWrite(int start, char* code, FILE* stream);
static void modificationRecordWrite(int start, int halfs, FILE* stream);
static void endRecordWrite(int excutable, FILE* stream);

void assemblerInitialize(Assembler* asmblr)
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
}

void assemblerAssemble(Assembler* asmblr, const char* filename)
{
	char buffer[256];
	sscanf(filename, "%[^.]", buffer);

	pass1(asmblr, buffer);
	pass2(asmblr, buffer);
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
	for (int i = 0; i< len; i++)
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
	char buffer[BUFFER_MAX];
	FILE* fp = fopen("./opcode.txt", "r");
	if (fp == NULL) {
		//
		return;
	}

	while (!feof(fp)) {
		fgets(buffer, BUFFER_MAX, fp);
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
		if (mne == NULL || code_ptr == NULL) {
			//
			return;
		}

		*code_ptr = code;
		hashInsert(&asmblr->op_table, mne, code_ptr);
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
		Entry* entry = (Entry*)data;
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
	char buffer[BUFFER_MAX];
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

	parseStatement(&stmt, buffer, BUFFER_MAX, fp_asm);
	if (!strcmp(stmt.instruction, "START")){
		if (!isHexadecimalStr(stmt.operand)) {
			// error
		}
		// save #[OPERAND] as starting address
		asmblr->start_addr = strtol(stmt.operand, NULL, 16);
		
		// write line to intermediate file
		parseStatement(&stmt, buffer, BUFFER_MAX, fp_asm);
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
					asmblr->error = ERR_DUPLICATE_SYMBOL;
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
			else if(!strcmp(stmt.instruction, "WORD")) {
				asmblr->locctr += 3;
			}
			else if (!strcmp(stmt.instruction, "RESW")) {
				if (isDecimalStr(stmt.operand)) {
					int operand = strtol(stmt.operand, NULL, 10);
					asmblr->locctr += 3 * operand;
				}
				else {
					// error
				}
			}
			else if (!strcmp(stmt.instruction, "RESB")) {
				if (isDecimalStr(stmt.operand)) {
					int operand = strtol(stmt.operand, NULL, 10);
					asmblr->locctr += operand;
				}
				else {
					// error
				}
			}
			else if (!strcmp(stmt.instruction, "BYTE")) {
				int len = strlen(stmt.instruction);

				if (stmt.instruction[0] == '\'' && stmt.instruction[len - 1] == '\'') {
					if (stmt.instruction[0] == 'C')
						asmblr->locctr += len - 3;
					else if (stmt.instruction[0] == 'X')
						asmblr->locctr += (len - 2) / 2;
					else {
						// error
					}
				}
				else {
					// error
				}
			}
		}

		// write int
		parseStatement(&stmt, buffer, BUFFER_MAX, fp_asm);
	}
	// write int
	asmblr->prog_len = asmblr->locctr - asmblr->start_addr;

	fclose(fp_asm);
	fclose(fp_int);
}

static void pass2(Assembler* asmblr, const char* filename)
{
	char buffer[BUFFER_MAX];
	char text_obj[TEXT_OBJ_LEN + 1] = { 0, };
	Statement stmt;

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

	parseStatement(&stmt, buffer, BUFFER_MAX, fp_int);
	if (!strcmp(stmt.instruction, "START")) {
		lstfileWrite(&stmt, fp_lst);
		parseStatement(&stmt, buffer, BUFFER_MAX, fp_int);
	}

	// write header record
	headerRecordWrite("", 0, 0, fp_obj);
	// init first text record
	while (strcmp(stmt.instruction, "END")) {
		if (!stmt.is_comment) {
			char obj_code[OBJ_MAX_LEN] = { 0, };

			void* opcode = hashGetValue(&asmblr->op_table, stmt.instruction);
			if (opcode != NULL) {
				if (stmt.has_operand) {
					void* operand = hashGetValue(&asmblr->sym_table, stmt.operand);
					if (operand != NULL) {
						// store symbol value as operand address
					}
					else {
						// store 0 as operand address
						// set error flag (undefined symbol)
					}
				}
				else {
					// store 0 as operand address
				}
				// assemble  the object code instruction
			}
			else if (!strcmp(stmt.instruction, "BYTE")) {
			}
			else if (!strcmp(stmt.instruction, "WORD")) {
				// convert constant to object code				
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
		parseStatement(&stmt, buffer, BUFFER_MAX, fp_int);
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

static void parseStatement(Statement* stmt, char* buffer, int maxCount, FILE* stream) {
	char* str;
	char* ptr;
	char* savePtr;

	if (buffer == NULL) {
		return;
	}
	
	fgets(buffer, maxCount, stream);

	// str = strTrimDup(buffer, buffer + strlen(buffer));
	str = strTrim(buffer, buffer + strlen(buffer));
	if (str == NULL) {
		return;
	}

	/* comment인지 검사 */
	if (str[0] == '.') {
		stmt->is_comment = true;
		stmt->comment = str;
		return;
	}


	//char* begin;
	//char* end;
	//strParse(str, &begin, &end);

	//char* begin;
	//char* end;
	//strParse(str, &begin, &end);

	//char* begin;
	//char* end;
	//strParse(str, &begin, &end);
	//if (begin == NULL) {

	//}


	char* dst = str;
	char* src = str;
	while (*src != 0) {
		if (*src == ' ' || *src == '\t' || *str == ',') {
			for (; *src == ' ' || *src == '\t' || *str == ','; src++)
				if (*src == ',')
			*(dst++) = ' ';
		}

	}
	*dst = *src;

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

static void lstfileWrite(Statement* stmt, FILE* stream)
{
	fprintf(stream, "%*d    ", LST_LINE_NUMBER_LEN, stmt->line_number);
	
	if (stmt->is_comment) {
		fprintf(stream, "%s\n", stmt->comment);
	}
	else {
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