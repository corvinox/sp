#include "sicxevm.h"
#include <string.h>
#include <stdlib.h>
#include "mytype.h"
#include "hash.h"

#define REGTAB_BUCKET_SIZE 16


static int registerHashFunc(void* key);
static int registerCompare(void* key0, void* key1);
static void registerRelease(void* data, void* aux);
static void registerLoad(HashTable* reg_tab);

BOOL vmInitialize(SICXEVM* vm)
{
	if (!vm)
		return false;

	/* 변수 값 초기화 */
	vm->initialized = false;
	vm->prog_addr = 0;

	/* sic/xe 머신에서 사용될 가상 메모리 할당 및 초기화 */
	vm->memory = (BYTE*)malloc(sizeof(BYTE) * MEMORY_SIZE);
	if (!vm->memory)
		return false;	
	memset(vm->memory, 0, sizeof(BYTE) * MEMORY_SIZE);

	/* Register 초기화 */
	hashInitialize(&vm->reg_table, REGTAB_BUCKET_SIZE, registerHashFunc, registerCompare);
	registerLoad(&vm->reg_table);
	
	/* intiailize 성공 */
	vm->initialized = true;
	return true;
}

void vmRelease(SICXEVM* vm)
{
	if (!vm)
		return;

	/* register talbe에 할당한 메모리 해제 */
	hashForeach(&vm->reg_table, NULL, registerRelease);
	hashRelease(&vm->reg_table);
	
	/* sic/xe 머신에서 사용될 가상 메모리 해제 */
	free(vm->memory);

	/* 변수 값 초기화 */
	vm->prog_addr = 0;
	vm->initialized = false;
}

BOOL vmIsInitialized(SICXEVM* vm)
{
	return vm && vm->initialized;
}

BYTE* vmGetMemory(SICXEVM* vm)
{
	if (!vm || !vm->initialized)
		return NULL;

	return vm->memory;
}

RegisterInfo* vmGetRegisterInfo(SICXEVM* vm, char* reg_name)
{
	if (!vm || !vm->initialized)
		return NULL;

	if (reg_name == NULL)
		return NULL;

	Register* reg = (Register*)hashGetValue(&vm->reg_table, reg_name);
	if (!reg)
		return NULL;

	return &reg->info;
}

WORD* vmGetRegisterValue(SICXEVM* vm, char* reg_name)
{
	if (!vm || !vm->initialized)
		return NULL;

	if (reg_name == NULL)
		return NULL;

	Register* reg = (Register*)hashGetValue(&vm->reg_table, reg_name);
	if (!reg)
		return NULL;

	return &reg->value;
}

HashTable* vmGetOpcodeTable(SICXEVM* vm)
{
	if (!vm || !vm->initialized)
		return NULL;

	return &vm->op_table;
}

extern void vmSetProgAddr(SICXEVM* loader, WORD prog_addr)
{

}

extern void vmSetBreakPoint(SICXEVM* loader, WORD addr)
{

}

extern void vmClearBreakPoints(SICXEVM* loader)
{

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
static int registerCompare(void* key0, void* key1)
{
	return strcmp((char*)key0, (char*)key1);
}

/*************************************************************************************
* 설명: register table은 hash table로 구성된다.
*       두 테이블의 각 entry의 key, value에 값을 넣을 때 메모리를 할당했으므로,
*       할당한 메모리를 해제해 주어야 한다. hash table의 모든 entry에 적용되는
*       action function으로 data에 할당한 메모리를 해제하는 역할을 한다.
* 인자:
* - data: hash table 각 entry를 나타내는 변수.
* - aux: 추가적으로 필요하면 활용하기 위한 변수. auxiliary
* 반환값: 없음
*************************************************************************************/
static void registerRelease(void* data, void* aux)
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
* 설명: SIC/XE 머신에서 사용되는 register 들의 mnemonic과 매핑되는 번호를 table에 저장.
* 인자:
* - asmblr: Assembler에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void registerLoad(HashTable* reg_tab)
{
	const Register REG_INITIALIZER[] = {
		{{ "A",  0 }, 0},
		{{ "X",  1 }, 0 },
		{{ "L",  2 }, 0 },
		{{ "B",  3 }, 0 },
		{{ "S",  4 }, 0 },
		{{ "T",  5 }, 0 },
		{{ "F",  6 }, 0 },
		{{ "PC", 8 }, 0 },
		{{ "SW", 9 }, 0 }
	};

	if (!reg_tab)
		return;

	int reg_size = sizeof REG_INITIALIZER / sizeof REG_INITIALIZER[0];
	for (int i = 0; i < reg_size; i++) {
		Register* reg = (Register*)calloc(1, sizeof(Register));
		reg->info.mnemonic = strdup(REG_INITIALIZER[i].info.mnemonic);
		reg->info.number = REG_INITIALIZER[i].info.number;
		reg->value = REG_INITIALIZER[i].value;
		hashInsert(reg_tab, reg->info.mnemonic, reg);
	}
}
