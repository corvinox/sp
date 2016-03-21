#ifndef SHELL_H_
#define SHELL_H_

#include "list.h"
#include "hash.h"

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif


#define OP_LEN_MAX 16;

#define MEM_SIZE 0xFFFFF
#define MEM_LINE 0x10

#define LINE_MAX    256
#define CMD_LEN_MAX 80
#define ARG_LEN_MAX 80
#define ARG_CNT_MAX 3

#define ERR_NONE        0
#define ERR_INIT        1
#define ERR_NO_CMD      2
#define ERR_INVALID_USE 3
#define ERR_RUN_FAIL    4
#define ERR_EMPTY       5

#define CMD_CNT 10
#define CMD_INVALID -1
#define CMD_HELP    0
#define CMD_DIR     1
#define CMD_QUIT    2
#define CMD_HISTORY 3
#define CMD_DUMP    4
#define CMD_EDIT    5
#define CMD_FILL    6
#define CMD_RESET   7
#define CMD_OPCODE  8
#define CMD_OPLIST  9

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
	int cmd_code;
	int argc;
	int error;
	int quit;
	int init;
	unsigned int mem_addr;

	char* vm;
	char cmd_line[LINE_MAX];
	char args[ARG_CNT_MAX][ARG_LEN_MAX];
	void(*cmds[CMD_CNT])(struct Shell_*);
	List history;
	HashTable op_table;
} Shell;

/* Shell 관련 함수 */
extern void initializeShell(Shell* shell);
extern void startShell(Shell* shell);
extern void releaseShell(Shell* shell);

#endif