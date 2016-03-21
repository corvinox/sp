#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

#define strdup _strdup

/* Command ���� �Լ� */
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
* ����: Shell ����ü�� ���� �ʱ�ȭ�� �����Ѵ�. ���� ���, ���� �������� ����
*       �ʱ�ȭ�ϰ�, ���� �޸𸮿� ���� �޸� �Ҵ��� �ϰ�, ������ �о� opcode table��
*       �����ϴ� ���� �۾��� �����Ѵ�. start�ϱ� ������ ������ �����ؾ� �Ѵ�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
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

	/* opcode�� ���� ������ ���� */
	parseOpcode(shell);

	/* �ʱ�ȭ ������ ��� ������ ������ ������ �ʱ�ȭ ���� */
	if (shell->error != ERR_NONE) {
		shell->init = false;
		printError(shell->error);
	}
	else {
		shell->init = true;
	}
}

/*************************************************************************************
* ����: ������ ���鼭 ����ڷκ��� ����� �Է¹ް� �Ľ��ϰ� �����ϴ� ���� �ݺ��Ѵ�.
����ڰ� quit ����� ���� �� ���� �ݺ��Ѵ�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
*************************************************************************************/
void startShell(Shell* shell)
{
	while (!shell->quit) {
		printf("sicsim>");

		/* ����ڷκ��� �Է��� �޾Ƽ� ��ɰ� ��� ���ڵ��� �Ľ� */
		parseCommandLine(shell);

		/* error�� ������ command ���� */
		if (shell->error == ERR_NONE)
			runCommand(shell);

		/* ���� �������� error�� ������ ��� */
		if (shell->error != ERR_NONE)
			printError(shell->error);

		shell->error = ERR_NONE;
	}
}

/*************************************************************************************
* ����: shell�� �Ҵ�� �޸𸮸� ��� �����Ѵ�. ���α׷� ���� ���� �ݵ�� �����Ѵ�.
���� �޸�, ����Ʈ, �ؽ����̺� ���� �޸𸮸� �����Ѵ�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
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
* ����: ��밡���� ��ɾ� ����� ���
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
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
* ����: ���� ���丮�� �ִ� ���丮�� ���ϵ��� ����� ���
*       ���� ������ ���� �̸� ������ '*'ǥ�ø�, ���丮�� '/'ǥ���Ѵ�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
*************************************************************************************/
void runCmdDir(Shell* shell)
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
	//	printf("���丮 ��θ� ���� ���߽��ϴ�.\n");
	//	shell->error = ERR_RUN_FAIL;
	//	return;
	//}

	//while ((entry = readdir(dp)) != NULL) {
	//	lstat(entry->d_name, &fs);

	//	/* . �� ..�� ���� */
	//	if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
	//		continue;

	//	printf("      %s", entry->d_name);

	//	/* ���丮�� ���� '/', ����Ӽ��� ������ '*'�� ǥ�� */
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
* ����: shell�� �����ϱ� ���� ��� shell ����ü�� quit���� true�� �Ͽ� shell��
*       ����ǵ��� �Ѵ�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
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
* ����: ������� ����� ��ɾ���� ������� ��ȣ�� �Բ� �����ش�.
*       ���� �ֱ� ����� ��ɾ ����Ʈ�� �ϴܿ� ���� �ȴ�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
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
* ����: shell�� �Ҵ�Ǿ� �ִ� �޸��� ������ Ư�� �������� ����Ѵ�.
* ����:
* - dump: �⺻������ 160 ����Ʈ�� ����Ѵ�.
*          dump�� �������� ��µ� ������ address�� shell->mem_addr �� �����ϰ� �ִ�.
*          �ٽ� dump�� �����Ű�� ������ ( address + 1 ) �������� ����Ѵ�.
*          dump ��ɾ ó�� ���۵� ���� 0 �������� ����Ѵ�.
*          dump�� ����� �� boundary check�� �Ͽ� exception error ó���Ѵ�.
* - dump start: start �������� 10������ ���.
* - dump start, end: start���� end���������� ������ ���.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
*************************************************************************************/
void runCmdDump(Shell* shell)
{
	int start_addr;
	int end_addr;
	int start_base;
	int end_base;
	int cur_addr;
	int cur_base;

	/* start�� end�� �������� �ʾ��� ��*/
	if (shell->argc == 0) {
		start_addr = shell->mem_addr;
		end_addr = start_addr + MEM_LINE * 10 - 1;
		if (end_addr >= MEM_SIZE)
			end_addr = MEM_SIZE - 1;
	}
	/* start�� end�� �������� ��*/
	else if (shell->argc == 2) {
		char* ptr;
		start_addr = (int)strtol(shell->args[0], &ptr, 16);
		if (*ptr != 0) {
			printf("%s: �߸��� ����\n", shell->args[0]);
			shell->error = ERR_RUN_FAIL;
			return;
		}

		end_addr = (int)strtoul(shell->args[1], &ptr, 16);
		if (*ptr != 0) {
			printf("%s: �߸��� ����\n", shell->args[1]);
			shell->error = ERR_RUN_FAIL;
			return;
		}
	}
	/* �ܴ̿� ���� */
	else {
		shell->error = ERR_INVALID_USE;
		return;
	}

	/* check range */
	if (start_addr < 0 || start_addr >= MEM_SIZE) {
		printf("%X: �ּҰ��� ��ȿ ����: [0, FFFFF] �� ������ϴ�.\n", start_addr);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (end_addr < 0 || end_addr >= MEM_SIZE) {
		printf("%X: �ּҰ��� ��ȿ ����: [0, FFFFF] �� ������ϴ�.\n", end_addr);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (start_addr > end_addr) {
		printf("�߸��� ����: ���� �ּҰ��� �� �ּҰ��� �ʰ��Ͽ����ϴ�.\n");
		shell->error = ERR_RUN_FAIL;
		return;
	}

	start_base = (start_addr / MEM_LINE) * MEM_LINE;
	end_base = (end_addr / MEM_LINE) * MEM_LINE;

	/* dump memory */
	for (cur_base = start_base; cur_base <= end_base; cur_base += MEM_LINE) {
		/* memory address */
		printf("%05X ", cur_base);

		/* 16���� ǥ�� */
		for (cur_addr = cur_base; cur_addr < cur_base + MEM_LINE; cur_addr++) {
			if (cur_addr >= start_addr && cur_addr <= end_addr)
				printf("%02X ", (unsigned char)shell->vm[cur_addr]);
			else
				printf("   ");
		}

		/* ASCII ǥ�� */
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

	/* ���� ���� */
	shell->mem_addr = (end_addr + 1) % MEM_SIZE;
}

/*************************************************************************************
* ����: �޸��� address������ ���� value�� ������ ������ �����Ѵ�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
*************************************************************************************/
void runCmdEdit(Shell* shell)
{
	char* ptr = NULL;
	int addr = 0;
	int value = 0;

	/* ���ڰ� 2���� �ƴϸ� ���� */
	if (shell->argc != 2) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	/* arguments �˻� �� 16������ ��ȯ */
	addr = (int)strtoul(shell->args[0], &ptr, 16);
	if (*ptr != 0) {
		printf("%s: �߸��� ����\n", shell->args[0]);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	value = (int)strtoul(shell->args[1], &ptr, 16);
	if (*ptr != 0) {
		printf("%s: �߸��� ����\n", shell->args[1]);
		shell->error = ERR_RUN_FAIL;
		return;
	}

	/* check range */
	if (addr < 0 || addr >= MEM_SIZE) {
		printf("%X: �ּҰ��� ��ȿ ����: [0, FFFFF] �� ������ϴ�.\n", addr);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (value < 0 || value > 255) {
		printf("%X: ���� ��ȿ ����: [0, FF] �� ������ϴ�.\n", value);
		shell->error = ERR_RUN_FAIL;
		return;
	}

	/* edit */
	shell->vm[addr] = value;
}

/*************************************************************************************
* ����: �޸��� start�������� end���������� ���� value�� ������ ������ �����Ѵ�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
*************************************************************************************/
void runCmdFill(Shell* shell)
{
	char* ptr;
	int start_addr = 0;
	int end_addr = 0;
	int value = 0;

	/* argument�� 3���� �ƴϸ� ���� */
	if (shell->argc != 3) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	/* arguments �˻� �� 16������ ��ȯ */
	start_addr = (int)strtoul(shell->args[0], &ptr, 16);
	if (*ptr != 0) {
		printf("%s: �߸��� ����\n", shell->args[0]);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	end_addr = (int)strtoul(shell->args[1], &ptr, 16);
	if (*ptr != 0) {
		printf("%s: �߸��� ����\n", shell->args[1]);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	value = (int)strtoul(shell->args[2], &ptr, 16);
	if (*ptr != 0) {
		printf("%s: �߸��� ����\n", shell->args[2]);
		shell->error = ERR_RUN_FAIL;
		return;
	}

	/* check range */
	if (start_addr < 0 || start_addr >= MEM_SIZE) {
		printf("%X: �ּҰ��� ��ȿ ����: [0, FFFFF] �� ������ϴ�.\n", start_addr);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (end_addr < 0 || end_addr >= MEM_SIZE) {
		printf("%X: �ּҰ��� ��ȿ ����: [0, FFFFF] �� ������ϴ�.\n", end_addr);
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (start_addr > end_addr) {
		printf("�߸��� ����: ���� �ּҰ��� �� �ּҰ��� �ʰ��Ͽ����ϴ�.\n");
		shell->error = ERR_RUN_FAIL;
		return;
	}
	if (value < 0 || value > 255) {
		printf("%X: ���� ��ȿ ����: [0, FF] �� ������ϴ�.\n", value);
		shell->error = ERR_RUN_FAIL;
		return;
	}

	/* fill */
	memset(shell->vm + start_addr, (char)value, sizeof(char) * (end_addr - start_addr + 1));
}

/*************************************************************************************
* ����: �޸� ��ü�� ���� 0���� �����Ų��.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
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
* ����: ���ڷ� ���� opcode�� mnemonic�� ���� code���� ����Ѵ�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
*************************************************************************************/
void runCmdOpcode(Shell* shell)
{
	if (shell->argc != 1) {
		shell->error = ERR_INVALID_USE;
		return;
	}

	int* code = (int*)getValue(&shell->op_table, shell->args[0]);
	if (code == NULL)
		printf("        �ش� ������ ã�� �� �����ϴ�.\n");
	else
		printf("        opcode is %X\n", *code);
}

/*************************************************************************************
* ����: ���� hash table�� ����� opcode�� ��� ����Ѵ�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
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
				printf(" �� [%s, %02X]", (char*)entry->key, *(int*)entry->value);
				ptr = ptr->next;
			}
		}
		printf("\n");
	}
}

/*************************************************************************************
* ����: shell�� ����� cmd_code�� �̿��Ͽ� �ش� code�� ���ε� �Լ��� ȣ��
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
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
* ����: �ش� ���� �ڵ忡 �ش��ϴ� ������ ���
* ����:
* - err_code: error�� ��Ÿ���� ����
* ��ȯ��: ����
*************************************************************************************/
static void printError(int err_code)
{
	switch (err_code) {
	case ERR_NONE:
	case ERR_EMPTY:
		break;

	case ERR_INIT:
		printf("Shell�� �ʱ�ȭ�ϴµ� �����߽��ϴ�. �����մϴ�.\n");
		break;

	case ERR_NO_CMD:
		printf("�� �� ���� ����Դϴ�. h[elp]�� �Է��Ͽ� ������ ����� Ȯ���ϼ���.\n");
		break;

	case ERR_INVALID_USE:
		printf("��� ���ڰ� �߸��Ǿ����ϴ�. h[elp]�� �Է��Ͽ� ������ Ȯ���ϼ���.\n");
		break;

	case ERR_RUN_FAIL:
		printf("����� ������ �� �����ϴ�.\n");
		break;

	default:
		printf("�� �� ���� ����\n");
		break;
	}
}


/*************************************************************************************
* ����: opcode.txt ���Ϸκ��� opcode�� ���� ����(code, mnemonic, type) ����
*       �о hash table�� �����Ѵ�. �� ������ �������� ���еȴٰ� �����ϰ�
*       strtok�� �̿��Ͽ� ������ �� ������ �о� �Ľ��Ѵ�. ���� �ٿ� ���ؼ�
*       �Ľ��ϴ� ���߿� ���ܰ� �߻��ϸ� �ش� ���� �ǳʶٰ� ���� ���� �д´�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
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
* ����: ����ڷκ��� �� ���� command-line�� �Է¹ް�, �Ľ��Ͽ� command�� argument��
*       ��´�. ���ܰ� �߻����� �ʴ´ٸ�, command-line�� ���� �����ϰ�, command��
*       ���� code���� �ִ� 3���� argument�� �����ϰ� �ȴ�.
* ����:
* - shell: shell�� ���� ������ ��� �ִ� ����ü�� ���� ������
* ��ȯ��: ����
*************************************************************************************/
static void parseCommandLine(Shell* shell)
{
	char buffer[LINE_MAX] = { 0, };
	char* ptr;
	char* ptr2;

	/* ���� �Է� */
	fgets(buffer, LINE_MAX, stdin);

	/* \n ���� */
	buffer[strlen(buffer) - 1] = 0;

	/* shell�� command line�� �����Ͽ� ���� */
	strncpy(shell->cmd_line, buffer, LINE_MAX);

	/* ��ɾ �Ľ�*/
	ptr = strtok(buffer, " \t");

	/* �� ���ڿ��� ��� ������ ó�� */
	if (ptr == NULL) {
		shell->error = ERR_EMPTY;
		return;
	}

	/* ��� ���ڿ��� �ڵ�� ��ȯ */
	shell->cmd_code = getCommandCode(ptr);
	if (shell->cmd_code == CMD_INVALID) {
		shell->error = ERR_NO_CMD;
		return;
	}

	/* ��ɿ� ���� ���ڸ� �Ľ� */
	ptr = strtok(NULL, "");
	if (ptr == NULL) {
		shell->argc = 0;
		return;
	}

	/* ���ڴ� ','�� ���� */
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
	/* argument�� 3���� ������ ���� */
	else {
		shell->error = ERR_INVALID_USE;
		return;
	}
}

/*************************************************************************************
* ����: �� ���ڿ��� ���۰� ���� �Է¹޾Ƽ� �� ���� �����ϴ� ������ ��� �����Ѵ�.
*       �߰��� �ִ� ������ �������� ����.
* ����:
* - start: ���ڿ��� ������ ����Ű�� ������
* - end: ���ڿ��� ���� ����Ű�� ������
* ��ȯ��: trim�� ����� ���ڿ��� ���� �����͸� ��ȯ
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
* ����: ��ɾ� ���ڿ��� �Է¹޾� ������ ��� code ���� ��ȯ�Ѵ�.
* ����:
* - cmd: ��ɾ� ���ڿ�
* ��ȯ��: �ش� ��ɾ� ���ڿ��� ���εǴ� �������� ��ȯ
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
	else
		return CMD_INVALID;
}

/*************************************************************************************
* ����: ���ڷ� ���޵� key�κ��� ������ hash ���� ����.
*       �⺻���� �����Ǵ� hash function �̴�.
* ����:
* - key: key
* ��ȯ��: �ش� key�� �̿��Ͽ� ����� hash ��
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
* ����: ������ key���� ������ hash table�� entry �߿��� ���� key�� ���� �ִ�
*       entry�� ã�� ���� �� �Լ��̴�.
* ����:
* - key0: entry�� key Ȥ�� ����ڰ� �˻��� ���ϴ� key. �������� ���ϹǷ� �����������.
* - key1: ����ڰ� �˻��� ���ϴ� key Ȥ�� entry�� key. �������� ���ϹǷ� �����������.
* ��ȯ��: ������ 0, �ٸ��� �� �̿��� ��
*************************************************************************************/
static int hashCmp(void* key0, void* key1)
{
	char* str_a = (char*)key0;
	char* str_b = (char*)key1;

	return strcmp(str_a, str_b);
}

/*************************************************************************************
* ����: history�� list�� �����Ǿ� �ִµ�, �ȿ� ��� data�� ���ڿ��� �Ҵ��Ͽ����Ƿ�,
*       �Ҵ��� �޸𸮸� ������ �־�� �Ѵ�. list �� ��� entry�� ����Ǵ�
*       action function���� data�� �Ҵ��� �޸𸮸� �����ϴ� ������ �Ѵ�.
* ����:
* - data: list �� item�� ������
* - aux: �߰������� �ʿ��ϸ� Ȱ���ϱ� ���� ����. auxiliary
* ��ȯ��: ����
*************************************************************************************/
static void releaseHistory(void* data, void* aux)
{
	if (data != NULL) {
		free(data);
	}
}

/*************************************************************************************
* ����: opcodelist�� hash table�� �����Ǵµ�, �� entry�� key�� ���ڿ��� �Ҵ������Ƿ�,
*       �Ҵ��� �޸𸮸� ������ �־�� �Ѵ�. hash table�� ��� entry�� ����Ǵ�
*       action function���� data�� �Ҵ��� �޸𸮸� �����ϴ� ������ �Ѵ�.
* ����:
* - data: list �� item�� ������
* - aux: �߰������� �ʿ��ϸ� Ȱ���ϱ� ���� ����. auxiliary
* ��ȯ��: ����
*************************************************************************************/
static void releaseOplist(void* data, void* aux)
{
	if (data != NULL) {
		Entry* entry = (Entry*)data;
		if (entry->key != NULL)
			free(entry->key);
		if (entry->value != NULL) {
			free(entry->value);
		}
		free(data);
	}
}