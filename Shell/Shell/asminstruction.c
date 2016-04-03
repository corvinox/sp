#include "asminstruction.h"
#include <stdio.h>
#include "assembler.h"
#include "strutil.h"
#include "hash.h"


/* buffer에 관련된 값 정의 */
#define BUFFER_LEN 1024


AsmInstruction* getInstruction(Assembler* asmblr, char* str)
{
	/* assmebler directive table에서 찾아보고 있으면 반환 */
	void* inst = hashGetValue(&asmblr->dir_table, str);
	if (inst != NULL)
		return (AsmInstruction*)inst;

	/* 없으면 opcode table에서 찾아보고 있으면 반환 */
	inst = hashGetValue(&asmblr->op_table, str);
	if (inst != NULL)
		return (AsmInstruction*)inst;

	return NULL;
}


void execInstStart(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	/* PASS1 일 때 기능 */
	if (asmblr->state == STAT_RUN_PASS1) {
		/* START instruction은 첫 라인에만 나타나야함 */
		if (stmt->line_number != 5) {
			/* error */
			fprintf(log_stream, "LINE %d: START는 중간에 사용될 수 없습니다.\n", stmt->line_number);
			return;
		}

		/* operand를 갖고 있지 않으면 에러 */
		if (!stmt->has_operand) {
			/* error */
			fprintf(log_stream, "LINE %d: START INSTRUCTION은 OPERAND가 필요합니다.\n", stmt->line_number);
			return;
		}

		BOOL error;
		int start_addr = strToInt(stmt->operand, 16, &error);
		if (error || start_addr < 0 || start_addr > 0xFFFFF) {
			/* error */
			fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
			return;
		}

		asmblr->pc_value = start_addr;
		asmblr->prog_addr = start_addr;
	}
	/* PASS2 일 때 기능 */
	else if (asmblr->state == STAT_RUN_PASS2) {
		/* 아무것도 안함*/
	}
}

void execInstEnd(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	/* PASS1 일 때 기능 */
	if (asmblr->state == STAT_RUN_PASS1) {
		/* program의 길이 계산 */
		asmblr->prog_len = asmblr->pc_value - asmblr->prog_addr;
	}
	/* PASS2 일 때 기능 */
	else if (asmblr->state == STAT_RUN_PASS2) {
		/* 아무것도 안함*/
	}
}

void execInstByte(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	/* PASS1 일 때 기능 */
	if (asmblr->state == STAT_RUN_PASS1) {
		int len = strlen(stmt->operand);

		if (stmt->operand[1] == '\'' && stmt->operand[len - 1] == '\'') {
			if (stmt->operand[0] == 'C')
				stmt->inst_len = len - 3;
			else if (stmt->operand[0] == 'X')
				stmt->inst_len = (len - 2) / 2;
			else {
				/* error */
				stmt->error = true;
				fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
			}
		}
		else {
			/* error */
			stmt->error = true;
			fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
		}
	}
	/* PASS2 일 때 기능 */
	else if (asmblr->state == STAT_RUN_PASS2) {
		/* 아무것도 안함*/
	}
}

void execInstWord(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	/* PASS1 일 때 기능 */
	if (asmblr->state == STAT_RUN_PASS1) {
		stmt->inst_len = 3;
	}
	/* PASS2 일 때 기능 */
	else if (asmblr->state == STAT_RUN_PASS2) {
		/* 아무것도 안함*/
	}
}

void execInstResb(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	/* PASS1 일 때 기능 */
	if (asmblr->state == STAT_RUN_PASS1) {
		BOOL error;
		int operand = strToInt(stmt->operand, 10, &error);
		if (error) {
			/* error */
			stmt->error = true; 
			fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
		}
		stmt->inst_len = operand;
	}
	/* PASS2 일 때 기능 */
	else if (asmblr->state == STAT_RUN_PASS2) {
		/* 아무것도 안함*/
	}
}

void execInstResw(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	/* PASS1 일 때 기능 */
	if (asmblr->state == STAT_RUN_PASS1) {
		BOOL error;
		int operand = strToInt(stmt->operand, 10, &error);
		if (error) {
			/* error */
			stmt->error = true;
			fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
		}
		stmt->inst_len = 3 * operand;
	}
	/* PASS2 일 때 기능 */
	else if (asmblr->state == STAT_RUN_PASS2) {
		/* 아무것도 안함*/
	}
}

void execInstBase(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	/* PASS1 일 때 기능 */
	if (asmblr->state == STAT_RUN_PASS1) {
		/* 아무것도 안함*/
	}
	/* PASS2 일 때 기능 */
	else if (asmblr->state == STAT_RUN_PASS2) {
		if (!stmt->has_operand) {
			/* error */
			stmt->error = true;
			fprintf(log_stream, "LINE %d: BASE INSTRUCTION은 OPERAND가 필요합니다.\n", stmt->line_number);
			return;
		}
		
	}
}

void execInstOpcode(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	/* PASS1 일 때 기능 */
	if (asmblr->state == STAT_RUN_PASS1) {
		stmt->inst_len = (stmt->is_extended ? 4 : (int)stmt->instruction->aux);
	}
	/* PASS2 일 때 기능 */
	else if (asmblr->state == STAT_RUN_PASS2) {

	}
}

static int operandForOpcodeParse(Statement* stmt, char* str, BOOL* error)
{
	int n = 0;
	int i = 0;
	int x = 0;
	int b = 0;
	int p = 0;
	int e = 0;

	int format = (int)stmt->instruction->aux;
	if (format == 2) {


	}

	///*  */
	//if (str[0] == '@')
	//	i = 1;
	//else if (str[0] == )
}