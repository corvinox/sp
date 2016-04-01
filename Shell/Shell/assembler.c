#include "assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include "strutil.h"
#include "asminstruction.h"
#include "asmpass1.h"
#include "asmpass2.h"

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


static int directiveHashFunc(void* key);
static int opcodeHashFunc(void* key);
static int symbolHashFunc(void* key);
static int registerHashFunc(void* key);
static int entryKeyCompare(void* key0, void* key1);
static int entryCompare(const void* a, const void* b);
static void entryRelease(void* data, void* aux); 

static void directiveLoad(Assembler* asmblr);
static void opcodeLoad(Assembler* asmblr);
static void registerLoad(Assembler* asmblr);


BOOL assemblerInitialize(Assembler* asmblr)
{
	/* assembler 변수 초기화 */
	asmblr->state = STAT_IDLE;
	asmblr->error = ERR_NO;
	asmblr->prog_len = 0;
	asmblr->start_addr = 0;
	asmblr->pc_value = 0;
	asmblr->in_filename[0] = 0;
	asmblr->int_filename[0] = 0;
	asmblr->lst_filename[0] = 0;
	asmblr->obj_filename[0] = 0;
	asmblr->prog_name[0] = 0;

	/* directive table 초기화 */
	hashInitialize(&asmblr->dir_table, DIRTAB_BUCKET_SIZE, directiveHashFunc, entryKeyCompare);

	/* opcode table 초기화 */
	hashInitialize(&asmblr->op_table, OPTAB_BUCKET_SIZE, opcodeHashFunc, entryKeyCompare);

	/* symbol table 초기화 */
	hashInitialize(&asmblr->sym_table, SYMTAB_BUCKET_SIZE, symbolHashFunc, entryKeyCompare);

	/* register table 초기화 */
	hashInitialize(&asmblr->reg_table, REGTAB_BUCKET_SIZE, registerHashFunc, entryKeyCompare);
	
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

void assemblerAssemble(Assembler* asmblr, const char* filename, FILE* log_stream)
{
	/* assembler start 상태로 전환 */
	asmblr->state = STAT_START;

	char buffer[256];
	sscanf(filename, "%[^.]", buffer);

	/* filename 설정 */
	sprintf(asmblr->in_filename, "%s", filename);
	sprintf(asmblr->int_filename, "%s.int", buffer);
	sprintf(asmblr->lst_filename, "%s.lst", buffer);
	sprintf(asmblr->obj_filename, "%s.obj", buffer);

	/* assemble 과정에 사용되는 변수 초기화 */
	asmblr->start_addr = 0;
	asmblr->prog_len = 0;
	sprintf(asmblr->prog_name, "");

	/* assemble 과정에 사용되는 symbol table 초기화 */
	hashForeach(&asmblr->sym_table, NULL, entryRelease);
	hashClear(&asmblr->sym_table);

	fprintf(log_stream, "assemble 시작\n");

	/* 2-pass 알고리즘 */
	/* pass1에서 에러가 발생하면 pass2는 실행하지 않음 */
	BOOL success = assemblePass1(asmblr, log_stream);
	if (success) {
		fprintf(log_stream, "first pass 성공\n");
		//success = assemblePass2(asmblr, log_stream);
		//if (success)
		//	fprintf(log_stream, "second pass 성공\n");
	}

	/* 파일 정리 */
	remove(asmblr->int_filename);
	if (success) {
		fprintf(log_stream, "assemble 성공\n");
	}
	else {
		remove(asmblr->lst_filename);
		remove(asmblr->obj_filename);
		fprintf(log_stream, "assemble 실패\n");
	}

	/* assembler finish 상태로 전환 */
	asmblr->state = STAT_FINISH;

	/* assembler idle 상태로 전환 */
	asmblr->state = STAT_IDLE;
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
					fprintf(stream, " → [%s, %02X]", (char*)entry->key, inst->value);
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

	for (int i = 0; i < asmblr->sym_table.size; i++) {
		if (entries[i] != NULL) {
			fprintf(stream, "\t%s\t%04X\n", (char*)entries[i]->key, *(int*)entries[i]->value);
		}
	}
	
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
static int entryKeyCompare(void* key0, void* key1)
{
	return strcmp((char*)key0, (char*)key1);
}

/*************************************************************************************
* 설명: qsort로 정렬 하기위해서 entry끼리 비교하는 함수. key를 기준으로 비교함.
* 인자:
* - a: entry.
* - b: entry.
* 반환값: 같으면 0, 다르면 그 이외의 값
*************************************************************************************/
static int entryCompare(const void* a, const void* b)
{
	HashEntry** e0 = (HashEntry**)a;
	HashEntry** e1 = (HashEntry**)b;
	return strcmp((*e1)->key, (*e0)->key);
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
		{ "START", INST_START, 0, NULL, execInstStart },
		{ "END",   INST_END,   0, NULL, execInstEnd   },
		{ "BYTE",  INST_BYTE,  0, NULL, execInstByte  },
		{ "WORD",  INST_WORD,  0, NULL, execInstWord  },
		{ "RESB",  INST_RESB,  0, NULL, execInstResb  },
		{ "RESW",  INST_RESW,  0, NULL, execInstResw  },
		{ "BASE",  INST_BASE,  0, NULL, execInstBase  },
	};

	for (int i = 0; i < 7; i++) {
		AsmInstruction* inst = (AsmInstruction*)malloc(sizeof(AsmInstruction));
		inst->mnemonic = strdup(inst_initializer[i].mnemonic); 
		inst->type = inst_initializer[i].type;
		inst->value = inst_initializer[i].value;
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
		inst->type = INST_OPCODE;
		inst->value = code;
		inst->aux = (void*)format;
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
		AsmRegister* reg = (AsmRegister*)calloc(1, sizeof(AsmRegister));
		reg->mnemonic = strdup(reg_initializer[i].mnemonic);
		reg->number= reg_initializer[i].number;
		hashInsert(&asmblr->reg_table, reg->mnemonic, reg);
	}
}
