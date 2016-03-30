#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include "strutil.h"

/*  */
#define DIRTAB_BUCKET_SIZE 11
#define OPTAB_BUCKET_SIZE  20
#define SYMTAB_BUCKET_SIZE 20
#define REGTAB_BUCKET_SIZE 16


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

static int directiveHashFunc(void* key);
static int opcodeHashFunc(void* key);
static int symbolHashFunc(void* key);
static int registerHashFunc(void* key);
static int entryCompare(const void* a, const void* b);
static void entryRelease(void* data, void* aux); 


static void directiveLoad(Assembler* asmblr);
static void opcodeLoad(Assembler* asmblr);
static void registerLoad(Assembler* asmblr);


static void pass1(Assembler* asmblr, const char* filename);
static void pass2(Assembler* asmblr, const char* filename);
static void parseStatement(Assembler* asmblr, Statement* stmt, FILE* stream);

static int getInstructionCode(Assembler* asmblr, char* str);
static void execInstStart(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux);
static void execInstEnd(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux);
static void execInstByte(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux);
static void execInstWord(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux);
static void execInstResb(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux);
static void execInstResw(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux);
static void execInstBase(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux);
static void execInstOpcode(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux);

static void intfileHeaderWrite(Statement* stmt, FILE* stream);
static void intfileWrite(Statement* stmt, FILE* stream);

static void lstfileWrite(Statement* stmt, FILE* stream);

static void headerRecordWrite(char* name, int start, int length, FILE* stream);
static void textRecordWrite(int start, char* code, FILE* stream);
static void modificationRecordWrite(int start, int halfs, FILE* stream);
static void endRecordWrite(int excutable, FILE* stream);

BOOL assemblerInitialize(Assembler* asmblr)
{
	/* register 정보 */
	//int idx = 0;
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

	/* directive table 초기화 */
	hashInitialize(&asmblr->dir_table, DIRTAB_BUCKET_SIZE, directiveHashFunc, entryCompare);

	/* opcode table 초기화 */
	hashInitialize(&asmblr->op_table, OPTAB_BUCKET_SIZE, opcodeHashFunc, entryCompare);

	/* symbol table 초기화 */
	hashInitialize(&asmblr->sym_table, SYMTAB_BUCKET_SIZE, symbolHashFunc, entryCompare);

	/* register table 초기화 */
	hashInitialize(&asmblr->reg_table, REGTAB_BUCKET_SIZE, registerHashFunc, entryCompare);
	
	/* assembler directive를 table에 추가 */
	directiveLoad(asmblr);

	/* opcode file에서 opcode 정보를 읽어서 opcode table에 저장 */
	opcodeLoad(asmblr);
	if (asmblr->error != ERR_NO)
		return false;

	/* SIC/XE 머신에서 사용되는 register들을 table에 저장 */
	registerLoad(asmblr);

	return true;
}

void assemblerRelease(Assembler* asmblr)
{
	/* directive table에 할당한 메모리 해제 */
	hashForeach(&asmblr->dir_table, NULL, entryRelease);
	hashRelease(&asmblr->dir_table);
	
	/* opcode talbe에 할당한 메모리 해제 */
	hashForeach(&asmblr->op_table, NULL, entryRelease);
	hashRelease(&asmblr->op_table);

	/* symbol talbe에 할당한 메모리 해제 */
	hashForeach(&asmblr->sym_table, NULL, entryRelease);
	hashRelease(&asmblr->sym_table);

	/* register talbe에 할당한 메모리 해제 */
	hashForeach(&asmblr->reg_table, NULL, entryRelease);
	hashRelease(&asmblr->reg_table);
}

void assemblerAssemble(Assembler* asmblr, const char* filename)
{
	FILE* fp = fopen(filename, "r");
	Statement stmt;

	char buffer[256];
	sscanf(filename, "%[^.]", buffer);


	hashForeach(&asmblr->sym_table, NULL, entryRelease);
	hashClear(&asmblr->sym_table);

	pass1(asmblr, buffer);

	//pass2(asmblr, buffer);
}

void assemblerPrintOpcode(Assembler* asmblr, char* opcode, FILE* stream)
{
	AsmInstruction* inst = (AsmInstruction*)hashGetValue(&asmblr->op_table, opcode);
	if (inst == NULL)
		fprintf(stream, "        해당 정보를 찾을 수 없습니다.\n");
	else
		fprintf(stream, "        opcode is %X\n", (int)inst->aux);
}

void assemblerPrintOpcodeTable(Assembler* asmblr, FILE* stream)
{
	for (int i = 0; i < asmblr->op_table.bucket_size; i++) {
		Node* ptr;
		fprintf(stream, "        %-2d : ", i + 1);

		ptr = asmblr->op_table.buckets[i].head;
		if (ptr != NULL) {
			HashEntry* entry = (HashEntry*)ptr->data;
			if (entry != NULL) {
				AsmInstruction* inst = (AsmInstruction*)entry->value;
				fprintf(stream, "[%s, %02X]", (char*)entry->key, (int)inst->aux);
			}
			
			ptr = ptr->next;

			while (ptr != NULL) {
				entry = (HashEntry*)ptr->data;
				if (entry != NULL) {
					AsmInstruction* inst = (AsmInstruction*)entry->value;
					fprintf(stream, " → [%s, %02X]", (char*)entry->key, (int)inst->aux);
				}
				ptr = ptr->next;
			}
		}
		fprintf(stream, "\n");
	}
}

void assemblerPrintSymbolTable(Assembler* asmblr, FILE* stream)
{
	HashEntry** entries = (HashEntry**)calloc(sizeof(HashEntry*), asmblr->sym_table.size);
	int idx = 0;
	for (int i = 0; i < asmblr->sym_table.bucket_size; i++) {
		Node* ptr = asmblr->sym_table.buckets[i].head;
		while (ptr != NULL) {
			entries[idx++] = (HashEntry*)ptr->data;
			ptr = ptr->next;
		}
	}

	qsort(entries, asmblr->sym_table.size, sizeof(HashEntry*), entryCompare);

	for (int i = 0; i < asmblr->sym_table.size; i++)
		fprintf(stream, "\t%s\t%04X\n", (char*)entries[i]->key, *(int*)entries[i]->value);
	
	free(entries);
}

/*************************************************************************************
* 설명: 인자로 전달된 key로부터 적절한 hash 값을 얻어낸다.
*       기본으로 제공되는 hash function 이다.
* 인자:
* - key: key
* 반환값: 해당 key를 이용하여 계산한 hash 값
*************************************************************************************/
static int directiveHashFunc(void* key)
{
	char* str = (char*)key;
	int sum = 0;
	int len = strlen(str);
	for (int i = 0; i < len; i++)
		sum += str[i];

	return sum % DIRTAB_BUCKET_SIZE;
}

/*************************************************************************************
* 설명: 인자로 전달된 key로부터 적절한 hash 값을 얻어낸다.
*       기본으로 제공되는 hash function 이다.
* 인자:
* - key: key
* 반환값: 해당 key를 이용하여 계산한 hash 값
*************************************************************************************/
static int opcodeHashFunc(void* key)
{
	char* str = (char*)key;
	int sum = 0;
	int len = strlen(str);
	for (int i = 0; i < len; i++)
		sum += str[i];

	return sum % OPTAB_BUCKET_SIZE;
}

/*************************************************************************************
* 설명: 인자로 전달된 key로부터 적절한 hash 값을 얻어낸다.
*       기본으로 제공되는 hash function 이다.
* 인자:
* - key: key
* 반환값: 해당 key를 이용하여 계산한 hash 값
*************************************************************************************/
static int symbolHashFunc(void* key)
{
	char* str = (char*)key;
	int sum = 0;
	int len = strlen(str);
	for (int i = 0; i < len; i++)
		sum += str[i];

	return sum % SYMTAB_BUCKET_SIZE;
}

/*************************************************************************************
* 설명: 인자로 전달된 key로부터 적절한 hash 값을 얻어낸다.
*       기본으로 제공되는 hash function 이다.
* 인자:
* - key: key
* 반환값: 해당 key를 이용하여 계산한 hash 값
*************************************************************************************/
static int registerHashFunc(void* key)
{
	char* str = (char*)key;
	int sum = 0;
	int len = strlen(str);
	for (int i = 0; i < len; i++)
		sum += str[i];

	return sum % REGTAB_BUCKET_SIZE;
}

/*************************************************************************************
* 설명: 임의의 key값을 넣으면 hash table의 entry 중에서 같은 key를 갖고 있는
*       entry를 찾기 위한 비교 함수이다.
* 인자:
* - key0: entry의 key 혹은 사용자가 검색을 원하는 key. 같은지만 비교하므로 순서상관없음.
* - key1: 사용자가 검색을 원하는 key 혹은 entry의 key. 같은지만 비교하므로 순서상관없음.
* 반환값: 같으면 0, 다르면 그 이외의 값
*************************************************************************************/
static int entryCompare(void* key0, void* key1)
{
	char* str_a = (char*)key0;
	char* str_b = (char*)key1;

	return strcmp(str_b, str_a);
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
static void entryRelease(void* data, void* aux)
{
	if (data != NULL) {
		HashEntry* entry = (HashEntry*)data;
		if (entry != NULL) {
			free(entry->key);
			free(entry->value);
		}
		free(entry);
	}
}

/*************************************************************************************
* 설명: assembler에서 사용되는 유사 instruction인 assembler directive 들을
*       hash table에 저장하여 초기화한다.
* 인자:
* - asmblr: Assembler에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void directiveLoad(Assembler* asmblr)
{
	AsmInstruction inst_initializer[] = {
		{ "START", INST_START, NULL, execInstStart },
		{ "END",   INST_END,   NULL, execInstEnd   },
		{ "BYTE",  INST_BYTE,  NULL, execInstByte  },
		{ "WORD",  INST_WORD,  NULL, execInstWord  },
		{ "RESB",  INST_RESB,  NULL, execInstResb  },
		{ "RESW",  INST_RESW,  NULL, execInstResw  },
		{ "BASE",  INST_BASE,  NULL, execInstBase  },
	};

	for (int i = 0; i < 7; i++) {
		AsmInstruction* inst = (AsmInstruction*)malloc(sizeof(AsmInstruction));
		inst->mnemonic = strdup(inst_initializer[i].mnemonic); 
		inst->inst_code = inst_initializer[i].inst_code;
		inst->aux = inst_initializer[i].aux;
		inst->exec_func = inst_initializer[i].exec_func;

		hashInsert(&asmblr->dir_table, inst->mnemonic, inst);
	}
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
	FILE* fp = fopen("./opcode.txt", "r");
	if (fp == NULL) {
		asmblr->error = ERR_NO_INIT;
		return;
	}

	int code;
	char buffer[BUFFER_LEN];
	while (!feof(fp)) {
		if (fgets(buffer, BUFFER_LEN, fp) == NULL)
			break;

		/* parse opcode */
		char* ptr = strtok(buffer, " \t");
		if (ptr == NULL)
			continue;

		char* endPtr;
		int code = (int)strtoul(ptr, &endPtr, 16);
		/* opcode에 대한 code는 1Byte 이내의 값이므로 1Byte가 아니면 */
		if (endPtr - ptr != 2)
			continue;
		/* opcode는 음수는 안됨 */
		else if (code < 0)
			continue;

		/* parse mnemonic */
		ptr = strtok(NULL, " \t");
		if (ptr == NULL)
			continue;

		char* mnemonic = ptr;

		/* parse format */
		ptr = strtok(NULL, " \t");
		if (ptr == NULL)
			continue;

		int format = 0;
		if (ptr[0] == '1' && ptr[1] == 0)
			format = 1;
		else if (ptr[0] == '2' && ptr[1] == 0)
			format = 2;
		else if (ptr[0] == '3' && ptr[1] == '/' && ptr[2] == '4')
			format = 3;
		else
			continue;

		AsmInstruction* inst = (AsmInstruction*)malloc(sizeof(AsmInstruction));
		inst->mnemonic = strdup(mnemonic);
		inst->inst_code = INST_OPCODE;
		inst->aux = (void*)code;
		inst->exec_func = execInstOpcode;

		hashInsert(&asmblr->op_table, inst->mnemonic, inst);
	}
}

/*************************************************************************************
* 설명: SIC/XE 머신에서 사용되는 register 들의 mnemonic과 매핑되는 번호를 table에 저장.
* 인자:
* - asmblr: Assembler에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void registerLoad(Assembler* asmblr)
{
	AsmRegister reg_initializer[] = {
		{ "A",  0 },
		{ "X",  1 },
		{ "L",  2 },
		{ "B",  3 },
		{ "S",  4 },
		{ "T",  5 },
		{ "F",  6 },
		{ "PC", 8 },
		{ "SW", 9 }
	};

	for (int i = 0; i < 9; i++) {
		AsmRegister* reg = (AsmRegister*)calloc(1, sizeof(AsmRegister*));
		reg->mnemonic = strdup(reg_initializer[i].mnemonic);
		reg->number= reg_initializer[i].number;

		hashInsert(&asmblr->reg_table, reg->mnemonic, reg);
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

	/*  line number 초기화 */
	stmt.line_number = 0;

	/* asm 파일에서 읽기 */
	parseStatement(asmblr, &stmt, fp_asm);
	//if (!strcmp(stmt.instruction, "START")) {
	if (stmt.inst_code == INST_START) {
		if (!isHexadecimalStr(stmt.operand)) {
			/* error */
			printf("        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, stmt.operand);
		}
		/* save #[OPERAND] as starting address */
		asmblr->start_addr = strtol(stmt.operand, NULL, 16);

		/* 중간 파일에 헤더 정보 쓰기 */
		intfileHeaderWrite(&stmt, fp_int);
		/* asm 파일에서 읽기 */
		parseStatement(asmblr, &stmt, fp_asm);
	}
	else {
		asmblr->start_addr = 0;
		asmblr->locctr = 0;
		/* 중간 파일에 헤더 정보 쓰기 */
		intfileHeaderWrite(&stmt, fp_int);
	}

	while (!feof(fp_asm)) {
		/* 주석 라인은 건너뜀 */
		if (!stmt.is_invalid && !stmt.is_comment) {
			/* label에 대한 처리 */
			if (stmt.has_label) {
				/* symbol table에서 label을 찾으면 중복 에러, 못찾으면 저장 */
				void* label = hashGetValue(&asmblr->sym_table, stmt.label);
				if (label != NULL) {
					/* error */
					asmblr->error = ERR_DUPLICATE_SYMBOL;
					printf("        line %d: 이미 존재하는 SYMBOL (%s)\n", stmt.line_number, stmt.label);
				}
				else {
					char* key = strdup(stmt.label);
					int* value = (int*)malloc(sizeof(int));
					*value = asmblr->locctr;

					/* insert (LABEL, LOCCTR) into SYMTAB */
					hashInsert(&asmblr->sym_table, key, value);
				}
			}
		}
	}


	//while (strcmp(stmt.instruction, "END")) {
	while (stmt.inst_code != INST_END) {
		/* 주석 라인은 건너뜀 */
		if (!stmt.is_invalid && !stmt.is_comment) {

			/* label에 대한 처리 */
			if (stmt.has_label) {
				/* symbol table에서 label을 찾으면 중복 에러, 못찾으면 저장 */
				void* label = hashGetValue(&asmblr->sym_table, stmt.label);
				if (label != NULL) {
					/* error */
					asmblr->error = ERR_DUPLICATE_SYMBOL;
					printf("        line %d: 이미 존재하는 SYMBOL (%s)\n", stmt.line_number, stmt.label);
				}
				else {
					char* key = strdup(stmt.label);
					int* value = (int*)malloc(sizeof(int));
					*value = asmblr->locctr;

					/* insert (LABEL, LOCCTR) into SYMTAB */
					hashInsert(&asmblr->sym_table, key, value);
				}
			}

			/* instruction 및 assembler directive 들에 대한 처리 */
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
					/* error */
					printf("        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, stmt.operand);
				}
			}
			else if (!strcmp(stmt.instruction, "RESB")) {
				if (isDecimalStr(stmt.operand)) {
					int operand = strtol(stmt.operand, NULL, 10);
					asmblr->locctr += operand;
				}
				else {
					/* error */
					printf("        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, stmt.operand);
				}
			}
			else if (!strcmp(stmt.instruction, "BYTE")) {
				int len = strlen(stmt.operand);

				if (stmt.operand[1] == '\'' && stmt.operand[len - 1] == '\'') {
					if (stmt.operand[0] == 'C')
						asmblr->locctr += len - 3;
					else if (stmt.operand[0] == 'X')
						asmblr->locctr += (len - 2) / 2;
					else {
						/* error */
						printf("        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, stmt.operand);
					}
				}
				else {
					/* error */
					printf("        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt.line_number, stmt.operand);
				}
			}

			/* 중간 파일에 쓰기 */
			intfileWrite(&stmt, fp_int);
		}
		/* asm 파일에서 읽기 */
		parseStatement(asmblr, &stmt, fp_asm);
	}
	/* 중간 파일에 쓰기 */
	intfileWrite(&stmt, fp_int);
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

static void execInstStart(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux)
{
	/* START instruction은 첫 라인에만 나타나야함 */
	if (stmt->line_number != 5) {
		/* error */
		fprintf(stream, "        line %d: START는 중간에 사용될 수 없습니다.\n", stmt->line_number);
	}
}

static void execInstEnd(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux)
{

}

static void execInstByte(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux)
{
	int len = strlen(stmt->operand);

	if (stmt->operand[1] == '\'' && stmt->operand[len - 1] == '\'') {
		if (stmt->operand[0] == 'C')
			asmblr->locctr += len - 3;
		else if (stmt->operand[0] == 'X')
			asmblr->locctr += (len - 2) / 2;
		else {
			/* error */
			fprintf(stream, "        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
		}
	}
	else {
		/* error */
		fprintf(stream, "        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
	}
}

static void execInstWord(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux)
{
	asmblr->locctr += 3;
}

static void execInstResb(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux)
{
	if (isDecimalStr(stmt->operand)) {
		int operand = strtol(stmt->operand, NULL, 10);
		asmblr->locctr += operand;
	}
	else {
		/* error */
		fprintf(stream, "        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
	}
}

static void execInstResw(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux)
{
	if (isDecimalStr(stmt->operand)) {
		int operand = strtol(stmt->operand, NULL, 10);
		asmblr->locctr += 3 * operand;
	}
	else {
		/* error */
		fprintf(stream, "        line %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
	}
}

static void execInstBase(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux)
{

}

static void execInstOpcode(Assembler* asmblr, Statement* stmt, FILE* stream, void* aux)
{
	asmblr->locctr += (stmt->is_extended ? 4 : 3);
}

static void intfileHeaderWrite(Statement* stmt, FILE* stream)
{
	fprintf(stream, "%d", stmt->line_number);
}

static void intfileWrite(Statement* stmt, FILE* stream)
{
	fprintf(stream, "%d %d\n", stmt->line_number, stmt->is_extended);
	fprintf(stream, "%s\n", stmt->label);
	fprintf(stream, "%s\n", stmt->instruction);
	fprintf(stream, "%s\n", stmt->operand);
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