#ifndef SICVM_H_
#define SICVM_H_

#include "mytype.h"

#define MEMORY_SIZE   0xFFFFF
#define REGISTER_SIZE 9

typedef struct _SICXEVM {
	BYTE memory[MEMORY_SIZE];
	WORD registers[REGISTER_SIZE];

} SICXEVM;

#endif
