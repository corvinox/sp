#include "parser.h"
#include <stdio.h>
#include "strutil.h"

void initializeParser(Parser* parser)
{
	initializeList(&parser->args);
}

void parseLine(Parser* parser, char* line)
{
	char* ptr;

	clearList(&parser->args);
	
	if (line == NULL)
		return;

	ptr = strtok(line, " \t\n");
	while (ptr != NULL) {
		addList(&parser->args, strdup(ptr));
		ptr = strtok(NULL, " \t\n");
	}
}