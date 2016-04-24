#ifndef SICXEVM_H_
#define SICXEVM_H_

#include "mytype.h"
#include "hash.h"

#define MEMORY_SIZE   0xFFFFF
#define REGISTER_COUNT 9
#define REGISTER_NUM_MIN 0
#define REGISTER_NUM_MAX 9
#define REGISTER_TABLE_SIZE (REGISTER_NUM_MAX - REGISTER_NUM_MIN + 1)


typedef struct _RegisterInfo
{
	char* mnemonic;
	int number;
} RegisterInfo;

typedef struct _Register
{
	RegisterInfo info;
	WORD value;
} Register;



typedef struct _SICXEVM 
{
	BOOL initialized;
	BYTE* memory;
	Register registers[REGISTER_COUNT];

	HashTable reg_table;
	HashTable op_table;
	HashTable instructionTable;

} SICXEVM;


extern BOOL vmInitialize(SICXEVM* vm);
extern void vmRelease(SICXEVM* vm);
extern BOOL vmIsInitialized(SICXEVM* vm);
extern BYTE* vmGetMemory(SICXEVM* vm);
extern RegisterInfo* vmGetRegisterInfo(SICXEVM* vm, char* reg_name);
extern WORD* vmGetRegisterValue(SICXEVM* vm, char* reg_name);
extern HashTable* vmGetInstructionTable(SICXEVM* vm);

#endif
