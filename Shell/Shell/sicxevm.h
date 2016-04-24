#ifndef SICXEVM_H_
#define SICXEVM_H_

#include "mytype.h"
#include "hash.h"

#define MEMORY_SIZE   0xFFFFF

typedef enum _OpcodeFormat
{
	FORMAT1,
	FORMAT2, 
	FORMAT34
} OpcodeFormat;

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

typedef struct _Opcode
{
	char* mnemonic;
	BYTE value;
	OpcodeFormat format;
} Opcode;

typedef struct _SICXEVM 
{
	BOOL initialized;
	WORD prog_addr;

	BYTE* memory;
	HashTable reg_table;
	HashTable op_table;

} SICXEVM;


extern BOOL vmInitialize(SICXEVM* vm);
extern void vmRelease(SICXEVM* vm);
extern BOOL vmIsInitialized(SICXEVM* vm);
extern BYTE* vmGetMemory(SICXEVM* vm);
extern RegisterInfo* vmGetRegisterInfo(SICXEVM* vm, char* reg_name);
extern WORD* vmGetRegisterValue(SICXEVM* vm, char* reg_name);
extern HashTable* vmGetOpcodeTable(SICXEVM* vm);
extern void vmSetProgAddr(SICXEVM* loader, WORD prog_addr);
extern void vmSetBreakPoint(SICXEVM* loader, WORD addr);
extern void vmClearBreakPoints(SICXEVM* loader);

#endif
