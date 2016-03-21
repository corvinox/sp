#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include "hash.h"
#include "list.h"

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

#define ERR_NONE        0
#define ERR_INIT        1
#define ERR_NO_CMD      2
#define ERR_INVALID_USE 3
#define ERR_RUN_FAIL    4
#define ERR_EMPTY       5

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

/* Command 관련 함수 */
void runCmdHelp(Shell* shell);
void runCmdDir(Shell* shell);
void runCmdQuit(Shell* shell);
void runCmdHistory(Shell* shell);
void runCmdDump(Shell* shell);
void runCmdEdit(Shell* shell);
void runCmdFill(Shell* shell);
void runCmdReset(Shell* shell);
void runCmdOpcode(Shell* shell);
void runCmdOplist(Shell* shell);
void runCommand(Shell* shell);

/* Shell 관련 함수 */
void initializeShell(Shell* shell);
void startShell(Shell* shell);
void releaseShell(Shell* shell);

static void printError(int err_code);
static void parseOpcode(Shell* shell);
static void parseCommandLine(Shell* shell);
static char* trim(char* start, char* end);
static int getCommandCode(char* cmd);
static int hashFunc(void* key);
static int hashCmp(void* a, void* b);
static void releaseHistory(void* data, void* aux);
static void releaseOplist(void* data, void* aux);




/*************************************************************************************
* 설명: 사용가능한 명령어 목록을 출력
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void runCmdHelp(Shell* shell) 
{
	if (shell->argc != 0) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	printf("        h[elp]\n");
	printf("        d[ir]\n");
	printf("        q[uit]\n");
	printf("        hi[story]\n");
	printf("        du[mp] [start, end]\n");
	printf("        e[dit] address, value\n");
	printf("        f[ill] start, end, value\n");
	printf("        reset\n");
	printf("        opcode mnemonic\n");
	printf("        opcodelist\n");
}

/*************************************************************************************
* 설명: 현재 디렉토리에 있는 디렉토리와 파일들의 목록을 출력
*       실행 파일은 파일 이름 옆에는 '*'표시를, 디렉토리는 '/'표시한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void runCmdDir(Shell* shell) 
{
	struct dirent *entry;
	struct stat fs;
	DIR *dp;
	int i = 0;

	if (shell->argc != 0) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	if ((dp = opendir(".")) == NULL) {
		printf("디렉토리 경로를 열지 못했습니다.\n");
		shell->error = ERR_RUN_FAIL;
		return;
	}

	while ((entry = readdir(dp)) != NULL) {
		lstat(entry->d_name, &fs);

		/* . 와 ..는 제외 */
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		printf("      %s", entry->d_name);

		/* 디렉토리면 끝에 '/', 실행속성을 가지면 '*'를 표시 */
		if (S_ISDIR(fs.st_mode))
			printf("/");
		else if (fs.st_mode&S_IEXEC)
			printf("*");
		if ((++i) % 3 == 0)
			printf("\n");
	}
	printf("\n");
	closedir(dp);
}

/*************************************************************************************
* 설명: shell을 종료하기 위한 명령 shell 구조체의 quit값을 true로 하여 shell이
*       종료되도록 한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void runCmdQuit(Shell* shell) 
{
	if (shell->argc != 0) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	shell->quit = true;
}

/*************************************************************************************
* 설명: 현재까지 사용한 명령어들을 순서대로 번호와 함께 보여준다.
*       가장 최근 사용한 명령어가 리스트의 하단에 오게 된다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void runCmdHistory(Shell* shell)
{
	int num = 0;
	Node* ptr;
	if (shell->argc != 0) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	for (ptr = shell->history.head; ptr != NULL; ptr = ptr->next) {
		char* cmd_line = (char*)ptr->data;
		printf("        %-5d%s\n", ++num, cmd_line);
	}
}

/*************************************************************************************
* 설명: shell에 할당되어 있는 메모리의 내용을 특정 형식으로 출력한다.
* 사용법:
* - dump: 기본적으로 160 바이트를 출력한다.
*          dump의 실행으로 출력된 마지막 address는 shell->mem_addr 에 저장하고 있다.
*          다시 dump를 실행시키면 마지막 ( address + 1 ) 번지부터 출력한다.
*          dump 명령어가 처음 시작될 때는 0 번지부터 출력한다.
*          dump가 출력할 시 boundary check를 하여 exception error 처리한다.
* - dump start: start 번지부터 10라인을 출력.
* - dump start, end: start부터 end번지까지의 내용을 출력.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void runCmdDump(Shell* shell) 
{
	int start_addr;
	int end_addr;
	int start_base;
	int end_base;
	int cur_addr;
	int cur_base;

	/* start와 end를 지정하지 않았을 때*/
	if (shell->argc == 0) {
		start_addr = shell->mem_addr;
		end_addr = start_addr + MEM_LINE * 10 - 1;
		if (end_addr >= MEM_SIZE)
			end_addr = MEM_SIZE - 1;
	}
	/* start와 end를 지정했을 때*/
	else if (shell->argc == 2) {
		char* ptr;
		start_addr = (int)strtol(shell->args[0], &ptr, 16);
		if (*ptr != 0) {
			printf("%s: 잘못된 인자\n", shell->args[0]);
			shell->error = ERR_RUN_FAIL;
			return;
		}

		end_addr = (int)strtoul(shell->args[1], &ptr, 16);
		if (*ptr != 0) {
			printf("%s: 잘못된 인자\n", shell->args[1]);
			shell->error = ERR_RUN_FAIL;
			return;
		}
	}
	/* 이외는 에러 */
	else {
		shell->error = ERR_INVALID_USE;
		return;
	}

	/* check range */
	if (start_addr < 0 || start_addr >= MEM_SIZE) {
		printf("%X: 주소값이 유효 범위: [0, FFFFF] 를 벗어났습니다.\n", start_addr);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (end_addr < 0 || end_addr >= MEM_SIZE) {
		printf("%X: 주소값이 유효 범위: [0, FFFFF] 를 벗어났습니다.\n", end_addr);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (start_addr > end_addr) {
		printf("잘못된 범위: 시작 주소값이 끝 주소값을 초과하였습니다.\n");
		shell->error = ERR_RUN_FAIL;
		return;
	}

	start_base = (start_addr / MEM_LINE) * MEM_LINE;
	end_base = (end_addr / MEM_LINE) * MEM_LINE;

	/* dump memory */
	for (cur_base = start_base; cur_base <= end_base; cur_base += MEM_LINE) {
		/* memory address */
		printf("%05X ", cur_base);

		/* 16진수 표기 */
		for (cur_addr = cur_base; cur_addr < cur_base + MEM_LINE; cur_addr++) {
			if (cur_addr >= start_addr && cur_addr <= end_addr)
				printf("%02X ", (unsigned char)shell->vm[cur_addr]);
			else
				printf("   ");
		}
	
		/* ASCII 표기 */
		printf("; ");
		for (cur_addr = cur_base; cur_addr < cur_base + MEM_LINE; cur_addr++) {
			char value = shell->vm[cur_addr];
			if (cur_addr >= start_addr && cur_addr <= end_addr && value >= 0x20 && value <= 0x7E)
				printf("%c", value);
			else
				printf(".");
		}
		
		printf("\n");
	}

	/* 내부 저장 */
	shell->mem_addr = (end_addr + 1) % MEM_SIZE;
}

/*************************************************************************************
* 설명: 메모리의 address번지의 값을 value에 지정된 값으로 변경한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void runCmdEdit(Shell* shell) 
{
	char* ptr = NULL;
	int addr = 0;
	int value = 0;

	/* 인자가 2개가 아니면 에러 */
	if (shell->argc != 2) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	/* arguments 검사 및 16진수로 변환 */
	addr = (int)strtoul(shell->args[0], &ptr, 16);
	if (*ptr != 0) {
		printf("%s: 잘못된 인자\n", shell->args[0]);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	value = (int)strtoul(shell->args[1], &ptr, 16);
	if (*ptr != 0) {
		printf("%s: 잘못된 인자\n", shell->args[1]);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	
	/* check range */
	if (addr < 0 || addr >= MEM_SIZE) {
		printf("%X: 주소값이 유효 범위: [0, FFFFF] 를 벗어났습니다.\n", addr);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (value < 0 || value > 255) {
		printf("%X: 값이 유효 범위: [0, FF] 를 벗어났습니다.\n", value);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	
	/* edit */
	shell->vm[addr] = value;
}

/*************************************************************************************
* 설명: 메모리의 start번지부터 end번지까지의 값을 value에 지정된 값으로 변경한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void runCmdFill(Shell* shell)
{
	char* ptr;
	int start_addr = 0;
	int end_addr = 0;
	int value = 0;

	/* argument가 3개가 아니면 에러 */
	if (shell->argc != 3) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	/* arguments 검사 및 16진수로 변환 */
	start_addr = (int)strtoul(shell->args[0], &ptr, 16);
	if (*ptr != 0) {
		printf("%s: 잘못된 인자\n", shell->args[0]);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	end_addr = (int)strtoul(shell->args[1], &ptr, 16);
	if (*ptr != 0) {
		printf("%s: 잘못된 인자\n", shell->args[1]);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	value = (int)strtoul(shell->args[2], &ptr, 16);
	if (*ptr != 0) {
		printf("%s: 잘못된 인자\n", shell->args[2]);
		shell->error = ERR_RUN_FAIL;
		return;
	}

	/* check range */
	if (start_addr < 0 || start_addr >= MEM_SIZE) {
		printf("%X: 주소값이 유효 범위: [0, FFFFF] 를 벗어났습니다.\n", start_addr);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (end_addr < 0 || end_addr >= MEM_SIZE) {
		printf("%X: 주소값이 유효 범위: [0, FFFFF] 를 벗어났습니다.\n", end_addr);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (start_addr > end_addr) {
		printf("잘못된 범위: 시작 주소값이 끝 주소값을 초과하였습니다.\n");
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (value < 0 || value > 255) {
		printf("%X: 값이 유효 범위: [0, FF] 를 벗어났습니다.\n", value);
		shell->error = ERR_RUN_FAIL;
		return;
	}

	/* fill */
	memset(shell->vm + start_addr, (char)value, sizeof(char) * (end_addr - start_addr + 1));
}

/*************************************************************************************
* 설명: 메모리 전체를 전부 0으로 변경시킨다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void runCmdReset(Shell* shell) 
{
	if (shell->argc != 0) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	memset(shell->vm, 0, sizeof(char) * MEM_SIZE);
}

/*************************************************************************************
* 설명: 인자로 받은 opcode의 mnemonic에 대한 code값을 출력한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void runCmdOpcode(Shell* shell)
{
	if (shell->argc != 1) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	int* code = (int*)getValue(&shell->op_table, shell->args[0]);
	if (code == NULL)
		printf("        해당 정보를 찾을 수 없습니다.\n");
	else
		printf("        opcode is %X\n", *code);
}

/*************************************************************************************
* 설명: 현재 hash table에 저장된 opcode를 모두 출력한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void runCmdOplist(Shell* shell)
{
	int i;
	if (shell->argc != 0) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	for (i = 0; i < BUCKET_SIZE; i++) {
		Node* ptr;
		printf("        %-2d : ", i + 1);

		ptr = shell->op_table.buckets[i].head;
		if (ptr != NULL) {
			Entry* entry = (Entry*)ptr->data;
			printf("[%s, %02X]", (char*)entry->key, *(int*)entry->value);
			ptr = ptr->next;

			while (ptr != NULL) {
				Entry* entry = (Entry*)ptr->data;
				printf(" → [%s, %02X]", (char*)entry->key, *(int*)entry->value);
				ptr = ptr->next;
			}
		}
		printf("\n");
	}
}

/*************************************************************************************
* 설명: shell에 저장된 cmd_code를 이용하여 해당 code에 매핑된 함수를 호출
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void runCommand(Shell* shell)
{
	if (shell->cmd_code < 0 || shell->cmd_code >= CMD_CNT) {
		shell->error = ERR_NO_CMD;
		return;
	}
	else {
		shell->cmds[shell->cmd_code](shell);
		if (shell->error == ERR_NONE)
			addList(&shell->history, strdup(shell->cmd_line));
	}
}

/*************************************************************************************
* 설명: Shell 구조체에 대한 초기화를 수행한다. 예를 들면, 각종 변수들의 값을 
*       초기화하고, 가상 메모리에 대한 메모리 할당을 하고, 파일을 읽어 opcode table을 
*       구성하는 등의 작업을 수행한다. start하기 전에는 무조건 실행해야 한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void initializeShell(Shell* shell) 
{
	int i;

	/* init variables */
	shell->cmd_code = CMD_INVALID;
	shell->argc = 0;
	shell->quit = false;
	shell->mem_addr = 0;
	shell->error = ERR_NONE;

	/* init virtual memory */
	for (i = 0; i < ARG_CNT_MAX; i++) {
		memset(shell->args[i], 0, sizeof(char) * ARG_LEN_MAX);
	}
	shell->vm = (char*)malloc(sizeof(char) * MEM_SIZE);
	if (shell->vm == NULL) {
		shell->error = ERR_INIT;
		return;
	}
	memset(shell->vm, 0, sizeof(char) * MEM_SIZE);


	/* init list & hash table*/
	initializeList(&shell->history);
	initializeHash(&shell->op_table, hashFunc, hashCmp);
	

	/* command function mapping */
	shell->cmds[CMD_HELP] = runCmdHelp;
	shell->cmds[CMD_DIR] = runCmdDir;
	shell->cmds[CMD_QUIT] = runCmdQuit;
	shell->cmds[CMD_HISTORY] = runCmdHistory;
	shell->cmds[CMD_DUMP] = runCmdDump;
	shell->cmds[CMD_EDIT] = runCmdEdit;
	shell->cmds[CMD_FILL] = runCmdFill;
	shell->cmds[CMD_RESET] = runCmdReset;
	shell->cmds[CMD_OPCODE] = runCmdOpcode;
	shell->cmds[CMD_OPLIST] = runCmdOplist;

	/* opcode에 대한 정보를 저장 */
	parseOpcode(shell);

	/* 초기화 과정을 모두 끝내고 에러가 있으면 초기화 실패 */
	if (shell->error != ERR_NONE) {	
		shell->init = false;
		printError(shell->error);
	}
	else {
		shell->init = true;
	}
}

/*************************************************************************************
* 설명: 루프를 돌면서 사용자로부터 명령을 입력받고 파싱하고 실행하는 것을 반복한다.
사용자가 quit 명령을 내릴 때 까지 반복한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void startShell(Shell* shell)
{
	while (!shell->quit) {
		printf("sicsim>");

		/* 사용자로부터 입력을 받아서 명령과 명령 인자들을 파싱 */
		parseCommandLine(shell);

		/* error가 없으면 command 실행 */
		if (shell->error == ERR_NONE)
			runCommand(shell);

		/* 위의 과정에서 error가 있으면 출력 */
		if (shell->error != ERR_NONE)
			printError(shell->error);

		shell->error = ERR_NONE;
	}
}

/*************************************************************************************
* 설명: shell에 할당된 메모리를 모두 해제한다. 프로그램 종료 전에 반드시 실행한다.
        가상 메모리, 리스트, 해쉬테이블 등의 메모리를 해제한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void releaseShell(Shell* shell)
{
	if (shell->vm != NULL)
		free(shell->vm);

	foreachList(&shell->history, NULL, releaseHistory);
	clearList(&shell->history);

	foreachHash(&shell->op_table, NULL, releaseOplist);
	clearHash(&shell->op_table);
}

/*************************************************************************************
* 설명: 해당 에러 코드에 해당하는 문구를 출력
* 인자:
* - err_code: error를 나타내는 정수
* 반환값: 없음
*************************************************************************************/
static void printError(int err_code)
{
	switch (err_code) {
	case ERR_NONE:
	case ERR_EMPTY:
		break;

	case ERR_INIT:
		printf("Shell을 초기화하는데 실패했습니다. 종료합니다.\n");
		break;

	case ERR_NO_CMD:
		printf("알 수 없는 명령입니다. h[elp]를 입력하여 가능한 명령을 확인하세요.\n");
		break;

	case ERR_INVALID_USE:
		printf("명령 인자가 잘못되었습니다. h[elp]를 입력하여 사용법을 확인하세요.\n");
		break;

	case ERR_RUN_FAIL:
		printf("명령을 실행할 수 없습니다.\n");
		break;

	default:
		printf("알 수 없는 오류\n");
		break;
	}
}

/*************************************************************************************
* 설명: opcode.txt 파일로부터 opcode에 대한 정보(code, mnemonic, type) 등을 
*       읽어서 hash table에 저장한다. 각 정보는 공백으로 구분된다고 가정하고
*       strtok를 이용하여 파일을 줄 단위로 읽어 파싱한다. 읽은 줄에 대해서 
*       파싱하는 도중에 예외가 발생하면 해당 줄은 건너뛰고 다음 줄을 읽는다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void parseOpcode(Shell* shell)
{
	char buffer[LINE_MAX];
	FILE* fp = fopen("./opcode.txt", "r");
	if (fp == NULL) {
		shell->error = ERR_INIT;
		return;
	}

	while (!feof(fp)) {
		char* ptr;
		char* mne;
		int* code_ptr;
		int code;

		fgets(buffer, LINE_MAX, fp);
		buffer[strlen(buffer) - 1] = 0;
		
		/* parse opcode */
		ptr = strtok(buffer, " \t");
		if (ptr == NULL)
			continue;
		code = (int)strtoul(ptr, NULL, 16);

		/* parse mnemonic */
		ptr = strtok(NULL, " \t");
		if (ptr == NULL)
			continue;

		mne = strdup(ptr);
		code_ptr = (int*)malloc(sizeof(int));
		if (mne == NULL || code_ptr == NULL) {
			shell->error = ERR_INIT;
			return;
		}

		*code_ptr = code;
		insertHash(&shell->op_table, mne, code_ptr);
	}
}

/*************************************************************************************
* 설명: 사용자로부터 한 줄의 command-line을 입력받고, 파싱하여 command와 argument를
*       얻는다. 예외가 발생하지 않는다면, command-line을 따로 저장하고, command에
*       대한 code값과 최대 3개의 argument를 저장하게 된다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void parseCommandLine(Shell* shell)
{
	char buffer[LINE_MAX] = { 0, };
	char* ptr;
	char* ptr2;

	/* 라인 입력 */
	fgets(buffer, LINE_MAX, stdin);

	/* \n 제거 */
	buffer[strlen(buffer) - 1] = 0;

	/* shell의 command line에 복사하여 저장 */
	strncpy(shell->cmd_line, buffer, LINE_MAX);

	/* 명령어를 파싱*/
	ptr = strtok(buffer, " \t");

	/* 빈 문자열인 경우 에러로 처리 */
	if (ptr == NULL) {
		shell->error = ERR_EMPTY;
		return;
	}

	/* 명령 문자열을 코드로 전환 */
	shell->cmd_code = getCommandCode(ptr);
	if (shell->cmd_code == CMD_INVALID) {
		shell->error = ERR_NO_CMD;
		return;
	}

	/* 명령에 대한 인자를 파싱 */
	ptr = strtok(NULL, "");
	if (ptr == NULL) {
		shell->argc = 0;
		return;
	}

	/* 인자는 ','로 구분 */
	/* argument0 */
	for (ptr2 = ptr; *ptr2 != 0 && *ptr2 != ','; ptr2++);
	if (*ptr2 == 0) {
		shell->argc = 1;
		strcpy(shell->args[0], trim(ptr, ptr2 - 1));
		return;
	}
	else {
		*ptr2 = 0;
		strcpy(shell->args[0], trim(ptr, ptr2 - 1));
		ptr = ptr2 + 1;
	}

	/* argument1 */
	for (ptr2 = ptr; *ptr2 != 0 && *ptr2 != ','; ptr2++);
	if (*ptr2 == 0) {
		shell->argc = 2;
		strcpy(shell->args[1], trim(ptr, ptr2 - 1));
		return;
	}
	else {
		*ptr2 = 0;
		strcpy(shell->args[1], trim(ptr, ptr2 - 1));
		ptr = ptr2 + 1;
	}

	/* argument2 */
	for (ptr2 = ptr; *ptr2 != 0 && *ptr2 != ','; ptr2++);
	if (*ptr2 == 0) {
		shell->argc = 3;
		strcpy(shell->args[2], trim(ptr, ptr2 - 1));
	}
	/* argument가 3개를 넘으면 에러 */
	else {
		shell->error = ERR_INVALID_USE;
		return;
	}
}

/*************************************************************************************
* 설명: 한 문자열의 시작과 끝을 입력받아서 양 끝에 존재하는 공백을 모두 제거한다.
*       중간에 있는 공백은 제거하지 않음.
* 인자:
* - start: 문자열의 시작을 가리키는 포인터
* - end: 문자열의 끝을 가리키는 포인터
* 반환값: trim이 적용된 문자열의 시작 포인터를 반환
*************************************************************************************/
static char* trim(char* start, char* end)
{
	char* ptr0;
	char* ptr1;

	for (ptr0 = start; isspace(*ptr0); ptr0++);
	for (ptr1 = end; isspace(*ptr1); ptr1--)
		*ptr1 = 0;

	return ptr0;
}

/*************************************************************************************
* 설명: 명령어 문자열을 입력받아 정수형 명령 code 값을 반환한다.
* 인자:
* - cmd: 명령어 문자열
* 반환값: 해당 명령어 문자열에 매핑되는 정수값을 반환
*************************************************************************************/
int getCommandCode(char* cmd)
{
	if (cmd == NULL)
		return CMD_INVALID;
	else if (!strncmp(cmd, "h", CMD_LEN_MAX) || !strncmp(cmd, "help", CMD_LEN_MAX))
		return CMD_HELP;
	else if (!strncmp(cmd, "d", CMD_LEN_MAX) || !strncmp(cmd, "dir", CMD_LEN_MAX))
		return CMD_DIR;
	else if (!strncmp(cmd, "q", CMD_LEN_MAX) || !strncmp(cmd, "quit", CMD_LEN_MAX))
		return CMD_QUIT;
	else if (!strncmp(cmd, "hi", CMD_LEN_MAX) || !strncmp(cmd, "history", CMD_LEN_MAX))
		return CMD_HISTORY;
	else if (!strncmp(cmd, "du", CMD_LEN_MAX) || !strncmp(cmd, "dump", CMD_LEN_MAX))
		return CMD_DUMP;
	else if (!strncmp(cmd, "e", CMD_LEN_MAX) || !strncmp(cmd, "edit", CMD_LEN_MAX))
		return CMD_EDIT;
	else if (!strncmp(cmd, "f", CMD_LEN_MAX) || !strncmp(cmd, "fill", CMD_LEN_MAX))
		return CMD_FILL;
	else if (!strncmp(cmd, "reset", CMD_LEN_MAX))
		return CMD_RESET;
	else if (!strncmp(cmd, "opcode", CMD_LEN_MAX))
		return CMD_OPCODE;
	else if (!strncmp(cmd, "opcodelist", CMD_LEN_MAX))
		return CMD_OPLIST;
	else
		return CMD_INVALID;
}

/*************************************************************************************
* 설명: 인자로 전달된 key로부터 적절한 hash 값을 얻어낸다.
*       기본으로 제공되는 hash function 이다.
* 인자:
* - key: key
* 반환값: 해당 key를 이용하여 계산한 hash 값
*************************************************************************************/
static int hashFunc(void* key)
{
	char* str = (char*)key;
	int i, sum;
	int len = strlen(str);
	for (i = 0, sum = 0; i< len; i++)
		sum += str[i];

	return sum % 20;
}

/*************************************************************************************
* 설명: 임의의 key값을 넣으면 hash table의 entry 중에서 같은 key를 갖고 있는
*       entry를 찾기 위한 비교 함수이다.
* 인자:
* - key0: entry의 key 혹은 사용자가 검색을 원하는 key. 같은지만 비교하므로 순서상관없음.
* - key1: 사용자가 검색을 원하는 key 혹은 entry의 key. 같은지만 비교하므로 순서상관없음.
* 반환값: 같으면 0, 다르면 그 이외의 값
*************************************************************************************/
static int hashCmp(void* key0, void* key1)
{
	char* str_a = (char*)key0;
	char* str_b = (char*)key1;
	
	return strcmp(str_a, str_b);
}

/*************************************************************************************
* 설명: history는 list로 구성되어 있는데, 안에 담긴 data로 문자열을 할당하였으므로,
*       할당한 메모리를 해제해 주어야 한다. list 의 모든 entry에 적용되는
*       action function으로 data에 할당한 메모리를 해제하는 역할을 한다.
* 인자:
* - data: list 각 item의 데이터
* - aux: 추가적으로 필요하면 활용하기 위한 변수. auxiliary
* 반환값: 없음
*************************************************************************************/
static void releaseHistory(void* data, void* aux)
{
	if (data != NULL) {
		free(data);
	}
}

/*************************************************************************************
* 설명: opcodelist는 hash table로 구성되는데, 각 entry의 key로 문자열을 할당했으므로,
*       할당한 메모리를 해제해 주어야 한다. hash table의 모든 entry에 적용되는
*       action function으로 data에 할당한 메모리를 해제하는 역할을 한다.
* 인자:
* - data: list 각 item의 데이터
* - aux: 추가적으로 필요하면 활용하기 위한 변수. auxiliary
* 반환값: 없음
*************************************************************************************/
static void releaseOplist(void* data, void* aux)
{
	if (data != NULL) {
		Entry* entry = (Entry*)data;
		if (entry->key != NULL)
			free(entry->key);
		if (entry->value!= NULL) {
			free(entry->value);
		}
		free(data);
	}	
}

int main() 
{ 
	Shell shell;
	
	/* 초기화 */
	initializeShell(&shell);

	/* shell이 초기화 되었으면 실행 */
	if (shell.init)
		startShell(&shell);

	/* shell이 종료되면 사용된 모든 자원을 해제 */
	releaseShell(&shell);

	return 0;
}
