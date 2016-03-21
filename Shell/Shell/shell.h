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
* ����: Shell�� ���� ������ ��� ����ü
* cmd_code: ����ڰ� �Է��� ��ɿ� ���εǴ� ������
* argc: ��ɿ� ���� ������ ����
* error: error�� ��Ÿ���� ������
* quit: shell ���� ���θ� ��Ÿ���� �÷���
* init: shell�� ���������� �ʱ�ȭ �Ǿ����� ���θ� ��Ÿ���� �÷���
* mem_addr: dump�� ���� ���������� �����ϴ� memory�� �ּҰ�
* vm: virtual memory
* cmd_line: ������� �Է�
* args: ��ɿ� ���� ���ڵ�
* cmds: ����� �����ϴ� �Լ��� ���� ������ �迭
* history: ���������� ����� ��ɿ� ���� command-line�� �����ϱ� ���� list
* op_table: opcode�� code, mnemonic, type�� ���� ������ ������ hash table
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

/* Shell ���� �Լ� */
extern void initializeShell(Shell* shell);
extern void startShell(Shell* shell);
extern void releaseShell(Shell* shell);

#endif