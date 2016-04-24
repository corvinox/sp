#include "sicxevm.h"
#include <string.h>
#include <stdlib.h>
#include "mytype.h"
#include "hash.h"

#define REGTAB_BUCKET_SIZE 16


static int registerHashFunc(void* key);
static int registerCompare(void* key0, void* key1);


BOOL vmInitialize(SICXEVM* vm)
{
	if (!vm)
		return false;
	vm->initialized = false;

	/* sic/xe �ӽſ��� ���� ���� �޸� �Ҵ� �� �ʱ�ȭ */
	vm->memory = (BYTE*)malloc(sizeof(BYTE) * MEMORY_SIZE);
	if (!vm->memory)
		return false;	
	memset(vm->memory, 0, sizeof(BYTE) * MEMORY_SIZE);

	hashInitialize(&vm->reg_table, REGTAB_BUCKET_SIZE, registerHashFunc, registerCompare);
	

	/* intiailize ���� */
	vm->initialized = true;
	return true;
}

void vmRelease(SICXEVM* vm)
{
	if (!vm)
		return;
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

HashTable* vmGetInstructionTable(SICXEVM* vm)
{
	if (!vm || !vm->initialized)
		return NULL;

	return &vm->instructionTable;
}


/*************************************************************************************
* ����: ���ڷ� ���޵� key�κ��� ������ hash ���� ����.
*       �⺻���� �����Ǵ� hash function �̴�.
* ����:
* - key: key
* ��ȯ��: �ش� key�� �̿��Ͽ� ����� hash ��
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
* ����: ������ key���� ������ hash table�� entry �߿��� ���� key�� ���� �ִ�
*       entry�� ã�� ���� �� �Լ��̴�.
* ����:
* - key0: entry�� key Ȥ�� ����ڰ� �˻��� ���ϴ� key. �������� ���ϹǷ� �����������.
* - key1: ����ڰ� �˻��� ���ϴ� key Ȥ�� entry�� key. �������� ���ϹǷ� �����������.
* ��ȯ��: ������ 0, �ٸ��� �� �̿��� ��
*************************************************************************************/
static int registerCompare(void* key0, void* key1)
{
	return strcmp((char*)key0, (char*)key1);
}

/*************************************************************************************
* ����: SIC/XE �ӽſ��� ���Ǵ� register ���� mnemonic�� ���εǴ� ��ȣ�� table�� ����.
* ����:
* - asmblr: Assembler�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
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