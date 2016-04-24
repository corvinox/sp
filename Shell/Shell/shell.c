#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
//#include <dirent.h>
//#include <sys/stat.h>

#include "strutil.h"

#define BUFFER_SIZE 12

/* Command 관련 함수 */
static void runCmdHelp(Shell* shell);
static void runCmdDir(Shell* shell);
static void runCmdQuit(Shell* shell);
static void runCmdHistory(Shell* shell);
static void runCmdDump(Shell* shell);
static void runCmdEdit(Shell* shell);
static void runCmdFill(Shell* shell);
static void runCmdReset(Shell* shell);
static void runCmdOpcode(Shell* shell);
static void runCmdOplist(Shell* shell);
static void runCmdAssemble(Shell* shell);
static void runCmdType(Shell* shell);
static void runCmdSymbol(Shell* shell);
static void runCmdProgaddr(Shell* shell);
static void runCmdLoader(Shell* shell);
static void runCmdRun(Shell* shell);
static void runCmdBp(Shell* shell);
static void commandInitialize(Shell* shell);
static void commandRun(Shell* shell);


static void errorPrint(int err_code);
static void parseOpcode(Shell* shell);
static void parseCommandLine(Shell* shell);
static int getCommandCode(char* cmd);
static void releaseHistory(void* data, void* aux);

/*************************************************************************************
* 설명: Shell 구조체에 대한 초기화를 수행한다. 예를 들면, 각종 변수들의 값을
*       초기화하고, 가상 메모리에 대한 메모리 할당을 하고, 파일을 읽어 opcode table을
*       구성하는 등의 작업을 수행한다. start하기 전에는 무조건 실행해야 한다.
*       인자로 넘어오는 구조체는 사전에 초기화되어있어야 한다.
*
* 인자:
* - sicxevm: sic/xe 가상 머신에 대한 정보를 담고 있는 구조체에 대한 포인터
* - asmblr: assembler에 대한 정보를 담고 있는 구조체에 대한 포인터
* - loader: loader에 대한 정보를 담고 있는 구조체에 대한 포인터
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 
* 반환값: 없음
*************************************************************************************/
void shellInitialize(Shell* shell, SICXEVM* sicxevm, Assembler* asmblr, Loader* loader)
{
	if (!shell || !sicxevm || !asmblr || !loader) {
		shell->error = ERR_INIT;
		return;
	}
	
	if (!vmIsInitialized(sicxevm) || !assemblerIsIntialized(asmblr) || loaderIsInitialized(loader)) {
		shell->error = ERR_INIT;
		return;
	}

	shell->sicxevm = sicxevm;
	shell->assembler = asmblr;
	shell->loader = loader;

	/* init variables */
	shell->argc = 0;
	shell->quit = false;
	shell->mem_addr = 0;
	shell->error = ERR_NONE;

	/* init virtual memory */
	for (int i = 0; i < ARG_CNT_MAX; i++) {
		memset(shell->args[i], 0, sizeof(char) * ARG_LEN_MAX);
	}
	
	/* init history*/
	listInitialize(&shell->history);

	/* init command */
	commandInitialize(shell);

	/* 초기화 과정을 모두 끝내고 에러가 있으면 초기화 실패 */
	if (shell->error != ERR_NONE) {
		shell->init = false;
		errorPrint(shell->error);
	}
	else {
		shell->init = true;
	}
}

/*************************************************************************************
* 설명: 루프를 돌면서 사용자로부터 명령을 입력받고 파싱하고 실행하는 것을 반복한다.
*       사용자가 quit 명령을 내릴 때 까지 반복한다. 초기화되지 않았으면 실행하지 않는다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void shellStart(Shell* shell)
{
	if (!shell->init)
		return;

	while (!shell->quit) {
		printf("sicsim>");

		/* 사용자로부터 입력을 받아서 명령과 명령 인자들을 파싱 */
		parseCommandLine(shell);

		/* error가 없으면 command 실행 */
		if (shell->error == ERR_NONE)
			commandRun(shell);

		/* 위의 과정에서 error가 있으면 출력 */
		if (shell->error != ERR_NONE)
			errorPrint(shell->error);

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
void shellRelease(Shell* shell)
{
	listForeach(&shell->history, NULL, releaseHistory);
	listClear(&shell->history);
}

/*************************************************************************************
* 설명: 사용가능한 명령어 목록을 출력
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void runCmdHelp(Shell* shell)
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
static void runCmdDir(Shell* shell)
{
	//struct dirent *entry;
	//struct stat fs;
	//DIR *dp;
	//int i = 0;

	//if (shell->argc != 0) {
	//	shell->error = ERR_INVALID_USE;
	//	return;
	//}

	//if ((dp = opendir(".")) == NULL) {
	//	printf("디렉토리 경로를 열지 못했습니다.\n");
	//	shell->error = ERR_RUN_FAIL;
	//	return;
	//}

	//while ((entry = readdir(dp)) != NULL) {
	//	lstat(entry->d_name, &fs);

	//	/* . 와 ..는 제외 */
	//	if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
	//		continue;

	//	printf("      %s", entry->d_name);

	//	/* 디렉토리면 끝에 '/', 실행속성을 가지면 '*'를 표시 */
	//	if (S_ISDIR(fs.st_mode))
	//		printf("/");
	//	else if (fs.st_mode&S_IEXEC)
	//		printf("*");
	//	if ((++i) % 3 == 0)
	//		printf("\n");
	//}
	//printf("\n");
	//closedir(dp);
}

/*************************************************************************************
* 설명: shell을 종료하기 위한 명령 shell 구조체의 quit값을 true로 하여 shell이
*       종료되도록 한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void runCmdQuit(Shell* shell)
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
static void runCmdHistory(Shell* shell)
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
static void runCmdDump(Shell* shell)
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
static void runCmdEdit(Shell* shell)
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
static void runCmdFill(Shell* shell)
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
static void runCmdReset(Shell* shell)
{
	if (shell->argc != 0) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	memset(shell->vm, 0, sizeof(char) * MEM_SIZE);
}

/*************************************************************************************
* 설명: 인자로 받은 opcode에 대한 정보를 출력하기 위해 assembler api를 호출한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void runCmdOpcode(Shell* shell)
{
	if (shell->argc != 1) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	assemblerPrintOpcode(shell->assembler, shell->args[0], stdout);
}

/*************************************************************************************
* 설명: 모든 opcode에 대한 정보를 출력하기 위해 assembler api를 호출한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void runCmdOplist(Shell* shell)
{
	if (shell->argc != 0) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	assemblerPrintOpcodeTable(shell->assembler, stdout);
}

/*************************************************************************************
* 설명: shell에 저장된 cmd_code를 이용하여 해당 code에 매핑된 함수를 호출
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void runCmdAssemble(Shell* shell)
{
	if (shell->argc != 1) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	assemblerAssemble(shell->assembler, shell->args[0], stdout);
}

/*************************************************************************************
* 설명: argument에 저장된 파일경로를 열고 내용을 화면에 출력한다. 파일을 열 수 없는
*       경우 에러 메세지를 출력한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void runCmdType(Shell* shell)
{
	FILE* fp;
	char buffer[BUFFER_SIZE];
	int indent;

	if (shell->argc != 1) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	fp = fopen(shell->args[0], "r");
	if (fp == NULL) {
		printf("        해당 파일을 열 수 없습니다.\n");
		shell->error = ERR_RUN_FAIL;
		return;
	}

	indent = true;
	while (!feof(fp)) {
		if (indent)
			printf("        ");

		if (fgets(buffer, BUFFER_SIZE, fp) == NULL) {
			printf("\n");
		}
		else {
			printf("%s", buffer);
			indent = (buffer[strlen(buffer) - 1] == '\n');
		}
	}
}

/*************************************************************************************
* 설명: shell에 저장된 cmd_code를 이용하여 해당 code에 매핑된 함수를 호출
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void runCmdSymbol(Shell* shell)
{
	if (shell->argc != 0) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	assemblerPrintSymbolTable(shell->assembler, stdout);
}

static void runCmdProgAddr(Shell* shell)
{	

}

static void runCmdLoader(Shell* shell)
{
	const char* EMPTY_STR = "                                                        \n";

	printf("            control     symbol      address     length  \n");
	printf("            section     name                            \n");
	printf("            --------------------------------------------\n");
	
	char format_str[64] = "                                                        \n";
	char num_str[16];
	char* sec_name = "asd";
	char* sym_name = "asd";
	int addr = 0;
	int length = 0;
	strncpy(format_str + 12, sec_name, strlen(sec_name));
	strncpy(format_str + 24, sym_name, strlen(sym_name));
	sprintf(num_str, "%04X", (WORD)addr);
	strncpy(format_str + 36, num_str, strlen(num_str));
	sprintf(num_str, "%04X", (WORD)length);
	strncpy(format_str + 48, num_str, strlen(num_str));
}

static void runCmdRun(Shell* shell)
{
	if (shell->argc != 0) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	/* 현재 Register들에 들어있는 값 출력 */
	printf("            %2s: %06X", "A", vmGetRegisterValue(shell->sicxevm, "A"));
	printf("  %2s: %06X\n", "X", vmGetRegisterValue(shell->sicxevm, "X"));
	printf("            %2s: %06X", "A", vmGetRegisterValue(shell->sicxevm, "L"));
	printf("  %2s: %06X\n", "X", vmGetRegisterValue(shell->sicxevm, "PC"));
	printf("            %2s: %06X", "A", vmGetRegisterValue(shell->sicxevm, "B"));
	printf("  %2s: %06X\n", "X", vmGetRegisterValue(shell->sicxevm, "S"));
	printf("            %2s: %06X", "A", vmGetRegisterValue(shell->sicxevm, "T"));
	printf("\n");
}

static void runCmdBreakPoint(Shell* shell)
{

}

/*************************************************************************************
* 설명: shell에 저장된 cmd_code를 초기화하고, command 함수를 매핑한다.
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void commandInitialize(Shell* shell)
{
	/* init command code */
	shell->cmd_code = CMD_INVALID;

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
	shell->cmds[CMD_ASSEMBLE] = runCmdAssemble;
	shell->cmds[CMD_TYPE] = runCmdType;
	shell->cmds[CMD_SYMBOL] = runCmdSymbol;
	shell->cmds[CMD_PROG_ADDR] = runCmdProgAddr;
	shell->cmds[CMD_LOADER] = runCmdLoader;
	shell->cmds[CMD_RUN] = runCmdRun;
	shell->cmds[CMD_BREAK_POINT] = runCmdBreakPoint;
}

/*************************************************************************************
* 설명: shell에 저장된 cmd_code를 이용하여 해당 code에 매핑된 함수를 호출
* 인자:
* - shell: shell에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
static void commandRun(Shell* shell)
{
	if (shell->cmd_code < 0 || shell->cmd_code >= CMD_CNT) {
		shell->error = ERR_NO_CMD;
		return;
	}
	else {
		shell->cmds[shell->cmd_code](shell);
		if (shell->error == ERR_NONE)
			listAdd(&shell->history, strdup(shell->cmd_line));
	}
}

/*************************************************************************************
* 설명: 해당 에러 코드에 해당하는 문구를 출력
* 인자:
* - err_code: error를 나타내는 정수
* 반환값: 없음
*************************************************************************************/
static void errorPrint(int err_code)
{
	switch (err_code) {
	case ERR_NONE:
	case ERR_EMPTY:
		break;

	case ERR_INIT:
		printf("        오류: Shell을 초기화하는데 실패했습니다. 종료합니다.\n");
		break;

	case ERR_NO_CMD:
		printf("        오류: 알 수 없는 명령입니다. h[elp]를 입력하여 가능한 명령을 확인하세요.\n");
		break;

	case ERR_INVALID_USE:
		printf("        오류: 명령 인자가 잘못되었습니다. h[elp]를 입력하여 사용법을 확인하세요.\n");
		break;

	case ERR_RUN_FAIL:
		printf("        오류: 명령을 실행할 수 없습니다.\n");
		break;

	default:
		printf("        오류: 알 수 없는 오류\n");
		break;
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
static void parseCommandLine(Shell* shell)
{
	char buffer[LINE_MAX] = { 0, };
	char* ptr;
	char* ptr2;

	/* 라인 입력 */
	if (fgets(buffer, LINE_MAX, stdin) == NULL) {
		shell->error = ERR_EMPTY;
		return;
	}


	/* trim */
	ptr = strTrim(buffer, buffer + strlen(buffer));

	/* shell의 command line에 복사하여 저장 */
	strncpy(shell->cmd_line, ptr, LINE_MAX);

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
		strcpy(shell->args[0], strTrim(ptr, ptr2 - 1));
		return;
	}
	else {
		*ptr2 = 0;
		strcpy(shell->args[0], strTrim(ptr, ptr2 - 1));
		ptr = ptr2 + 1;
	}

	/* argument1 */
	for (ptr2 = ptr; *ptr2 != 0 && *ptr2 != ','; ptr2++);
	if (*ptr2 == 0) {
		shell->argc = 2;
		strcpy(shell->args[1], strTrim(ptr, ptr2 - 1));
		return;
	}
	else {
		*ptr2 = 0;
		strcpy(shell->args[1], strTrim(ptr, ptr2 - 1));
		ptr = ptr2 + 1;
	}

	/* argument2 */
	for (ptr2 = ptr; *ptr2 != 0 && *ptr2 != ','; ptr2++);
	if (*ptr2 == 0) {
		shell->argc = 3;
		strcpy(shell->args[2], strTrim(ptr, ptr2 - 1));
	}
	/* argument가 3개를 넘으면 에러 */
	else {
		shell->error = ERR_INVALID_USE;
		return;
	}
}

/*************************************************************************************
* 설명: 명령어 문자열을 입력받아 정수형 명령 code 값을 반환한다.
* 인자:
* - cmd: 명령어 문자열
* 반환값: 해당 명령어 문자열에 매핑되는 정수값을 반환
*************************************************************************************/
static int getCommandCode(char* cmd)
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
	else if (!strncmp(cmd, "assemble", CMD_LEN_MAX))
		return CMD_ASSEMBLE;
	else if (!strncmp(cmd, "type", CMD_LEN_MAX))
		return CMD_TYPE;
	else if (!strncmp(cmd, "symbol", CMD_LEN_MAX))
		return CMD_SYMBOL;
	else if (!strncmp(cmd, "progaddr", CMD_LEN_MAX))
		return CMD_PROG_ADDR;
	else if (!strncmp(cmd, "loader", CMD_LEN_MAX))
		return CMD_LOADER;
	else if (!strncmp(cmd, "run", CMD_LEN_MAX))
		return CMD_RUN;
	else if (!strncmp(cmd, "bp", CMD_LEN_MAX))
		return CMD_BREAK_POINT;
	else
		return CMD_INVALID;
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