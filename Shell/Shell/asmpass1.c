﻿#include "asmpass1.h"
#include <stdio.h>
#include <stdlib.h>
#include "mytype.h"
#include "assembler.h"
#include "strutil.h"

/* buffer에 관련된 값 정의 */
#define BUFFER_LEN 1024

static void statementParse(Assembler* asmblr, Statement* stmt, FILE* stream);
static AsmInstruction* getInstruction(Assembler* asmblr, char* str);
static void intfileWrite(Statement* stmt, FILE* stream);


BOOL assemblePass1(Assembler* asmblr, FILE* log_stream)
{
	Statement stmt = { 0, };

	FILE* fp_asm = fopen(asmblr->in_filename, "r");
	FILE* fp_int = fopen(asmblr->int_filename, "w");
	if (fp_asm == NULL || fp_int == NULL) {
		if (fp_asm)
			fclose(fp_asm);
		if (fp_int)
			fclose(fp_int);
		return;
	}

	BOOL pass_error = false;
	BOOL pass_end = false;

	/* 라인 넘버 초기화 */
	stmt.line_number = 0;

	while (!feof(fp_asm) && stmt.inst_code != INST_END) {
		/* statement 초기화 */
		stmt.loc = asmblr->locctr;
		stmt.error = false;
		stmt.is_comment = false;
		stmt.is_empty = false;
		stmt.is_extended = false;
		stmt.is_indexed = false;
		stmt.is_invalid = false;
		stmt.has_label = false;
		stmt.instruction = NULL;
		stmt.label[0] = 0;
		stmt.operand[0] = 0;

		/* asm 파일에서 읽기 */
		statementParse(asmblr, &stmt, fp_asm);

		/* 주석과 빈 라인은 건너뜀 */
		if (stmt.is_empty || stmt.is_comment)
			continue;

		/* End directive 이후에도 statement가 존재한다면 */
		if (pass_end) {
			/* error */
			fprintf(log_stream, "LINE %d: END가 포함된 STATEMENT가 프로그램의 끝이어야 합니다.\n", stmt.line_number);
			pass_error = true;
			break;
		}

		/* 주석이나 빈 라인이 아니라면 라인 증가*/
		stmt.line_number += 5;

		/* label에 대한 처리 */
		if (stmt.has_label) {
			/* symbol table에서 label을 찾으면 중복 에러, 못찾으면 저장 */
			void* label = hashGetValue(&asmblr->sym_table, stmt.label);
			if (label != NULL) {
				/* error */
				asmblr->error = ERR_DUPLICATE_SYMBOL;
				stmt.error = true;
				fprintf(log_stream, "LINE %d: SYMBOL(%s)이 이미 존재합니다.\n", stmt.line_number, stmt.label);
			}
			else {
				char* key = strdup(stmt.label);
				int* value = (int*)malloc(sizeof(int));
				*value = asmblr->locctr;

				/* insert (LABEL, LOCCTR) into SYMTAB */
				hashInsert(&asmblr->sym_table, key, value);
			}
		}

		/* instruction 및 assembler directive 들에 대한 처리 */
		if (stmt.instruction == NULL) {
			/* error: instruction 없음 */
			stmt.error = true;
			fprintf(log_stream, "LINE %d: INSTRUCTION이 존재하지 않는 STATEMENT입니다.\n", stmt.line_number);
		}
		else if (stmt.instruction->exec_func == NULL) {
			/* error: instruction 없음 */
			stmt.error = true;
			fprintf(log_stream, "LINE %d: 예상치 못한 오류로 해당 INSTRUCTION을 처리할 수 없습니다.\n", stmt.line_number);
		}
		else {
			/* 각 instruction 및 assembler directive에 대한 처리 함수 실행 */
			stmt.instruction->exec_func(asmblr, &stmt, log_stream);
			if (!strcmp(stmt.instruction->mnemonic, "END"))
				pass_error = true;
		}
		
		if (stmt.error) {
			/* statement에 에러가 발생했으면, pass_error를 true로 설정 */
			pass_error = true;
		}
		if (!stmt.error) {
			/* statement에 에러가 없다면 중간 파일에 씀 */
			intfileWrite(&stmt, fp_int);
		}
	}

	/* End 문이 없음 */
	if (asmblr->prog_len == 0) {
		fprintf(log_stream, "LINE %d: END STATEMENT가 존재하지 않습니다.\n", stmt.line_number);
		pass_error = true;
	}

	fclose(fp_asm);
	fclose(fp_int);

	return !pass_error;
}

void execInstStart(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
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

	asmblr->locctr = start_addr;
	asmblr->start_addr = start_addr;
}

void execInstEnd(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	/* program의 길이 계산 */
	asmblr->prog_len = asmblr->locctr - asmblr->start_addr;
}

void execInstByte(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	int len = strlen(stmt->operand);

	if (stmt->operand[1] == '\'' && stmt->operand[len - 1] == '\'') {
		if (stmt->operand[0] == 'C')
			asmblr->locctr += len - 3;
		else if (stmt->operand[0] == 'X')
			asmblr->locctr += (len - 2) / 2;
		else {
			/* error */
			fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
		}
	}
	else {
		/* error */
		fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
	}
}

void execInstWord(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	asmblr->locctr += 3;
}

void execInstResb(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	BOOL error;
	int operand = strToInt(stmt->operand, 10, &error);
	if (error) {
		/* error */
		fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
	}
	asmblr->locctr += operand;
}

void execInstResw(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	BOOL error;
	int operand = strToInt(stmt->operand, 10, &error);
	if (error) {
		/* error */
		fprintf(log_stream, "LINE %d: 잘못된 OPERAND 오류 (%s)\n", stmt->line_number, stmt->operand);
	}
	asmblr->locctr += 3 * operand;
}

void execInstBase(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{

}

void execInstOpcode(Assembler* asmblr, Statement* stmt, FILE* log_stream)
{
	asmblr->locctr += (stmt->is_extended ? 4 : (int)stmt->instruction->aux);
}

static void statementParse(Assembler* asmblr, Statement* stmt, FILE* stream)
{
	char buffer[BUFFER_LEN] = { 0, };
	char buffer2[BUFFER_LEN] = { 0, };
	if (fgets(buffer, BUFFER_LEN, stream) == NULL) {
		/* 파일의 끝이면 현재 statement가 빈 문자열인 것으로 설정 */
		stmt->is_empty = true;
		return;
	}

	/* 앞,뒤 공백을 제거 */
	char* begin = strTrim(buffer, buffer + strlen(buffer));
	char* end = NULL;

	if (strlen(begin) == 0) {
		/* 빈 문자열인 경우 */
		stmt->is_empty = true;
		return;
	}

	if (begin[0] == '.') {
		/* comment인 경우 */
		stmt->is_comment = true;
		return;
	}

	strParse(begin, &begin, &end);
	strncpy(buffer2, begin, end - begin);
	buffer2[end - begin] = 0;
	
	/* 파싱한 첫 문자열이 instruction이 아니면 label로 인식 */
	stmt->instruction = getInstruction(asmblr, (buffer2[0] == '+' ? buffer2 + 1 : buffer2));
	if (stmt->instruction == NULL) {
		/* label flag를 셋 */
		stmt->has_label = true;
		strcpy(stmt->label, buffer2);

		strParse(end, &begin, &end);
		strncpy(buffer2, begin, end - begin);
		buffer2[end - begin] = 0;

		/* label 뒤에 파싱한 문자열에서 instruction을 추출 */
		stmt->instruction = getInstruction(asmblr, (buffer2[0] == '+' ? buffer2 + 1 : buffer2));
		if ( stmt->instruction != NULL) {
			/* Instruction을 파싱했는데, 만약 앞에 +가 붙어 있으면 확장 포맷 */
			stmt->is_extended = (BOOL)(buffer2[0] == '+');
		}
	}
	else {
		/* Instruction을 파싱했는데, 만약 앞에 +가 붙어 있으면 확장포맷 */
		stmt->is_extended = (BOOL)(buffer2[0] == '+');
	}

	/* operand 설정 */
	begin = strTrimFront(end);
	int operand_len = strlen(begin);
	if (operand_len > 0) {
		/* operand 설정 */
		stmt->has_operand = true;
		strcpy(stmt->operand, begin);
	}
}

static AsmInstruction* getInstruction(Assembler* asmblr, char* str)
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

static void intfileWrite(Statement* stmt, FILE* stream)
{
	fprintf(stream, "%d %d\n", stmt->line_number, stmt->loc);
	fprintf(stream, "%s\n", stmt->label);
	if (stmt->is_extended)
		fprintf(stream, "+");
	fprintf(stream, "%s\n", stmt->instruction->mnemonic);
	fprintf(stream, "%s\n", stmt->operand);
}