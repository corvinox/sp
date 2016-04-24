#ifndef SHELL_H_
#define SHELL_H_

#include "mytype.h"
#include "list.h"
#include "hash.h"
#include "sicxevm.h"
#include "loader.h"
#include "assembler.h"

#define OP_LEN_MAX 16;

#define MEM_SIZE 0xFFFFF
#define MEM_LINE 0x10

#define LINE_MAX    256
#define CMD_LEN_MAX 80
#define ARG_LEN_MAX 80
#define ARG_CNT_MAX 3

typedef enum
{ 
	ERR_NONE, 
	ERR_INIT, 
	ERR_NO_CMD, 
	ERR_INVALID_USE, 
	ERR_RUN_FAIL, 
	ERR_EMPTY 
} ShellError;

#define CMD_CNT 13
typedef enum
{ 
	CMD_INVALID = -1, 
	CMD_HELP, 
	CMD_DIR, 
	CMD_QUIT, 
	CMD_HISTORY, 
	CMD_DUMP, 
	CMD_EDIT, 
	CMD_FILL, 
	CMD_RESET, 
	CMD_OPCODE, 
	CMD_OPLIST,

	CMD_ASSEMBLE,
	CMD_TYPE,
	CMD_SYMBOL,

	CMD_PROG_ADDR,
	CMD_LOADER,
	CMD_RUN,
	CMD_BREAK_POINT
} ShellCmd;


/*************************************************************************************
* 설명: Shell에 대한 정보를 담는 구조체
* cmd_code: 사용자가 입력한 명령에 매핑되는 정수값
* argc: 명령에 대한 인자의 갯수
* error: error를 나타내는 정수값
* quit: shell 종료 여부를 나타내는 플래그
* init: shell이 정상적으로 초기화 되었는지 여부를 나타내는 플래그
* mem_addr: dump를 위해 내부적으로 저장하는 memory의 주소값
* vm: virtual memory
* cmd_line: 사용자의 입력
* args: 명령에 대한 인자들
* cmds: 명령을 실행하는 함수에 대한 포인터 배열
* history: 정상적으로 실행된 명령에 대한 command-line을 저장하기 위한 list
* op_table: opcode의 code, mnemonic, type에 대한 정보를 저장할 hash table
*************************************************************************************/
typedef struct Shell_ {
	ShellError error;
	ShellCmd cmd_code;
	int argc;
	
	BOOL quit;
	BOOL init;
	unsigned int mem_addr;

	char* vm;
	char cmd_line[LINE_MAX];
	char args[ARG_CNT_MAX][ARG_LEN_MAX];
	void(*cmds[CMD_CNT])(struct Shell_*);
	List history;
	HashTable op_table;

	SICXEVM* sicxevm;
	Assembler* assembler;
	Loader* loader;
} Shell;

/* Shell 관련 함수 */
extern void shellInitialize(Shell* shell, SICXEVM* sicxevm, Assembler* asmblr, Loader* loader);
extern void shellStart(Shell* shell);
extern void shellRelease(Shell* shell);

#endif