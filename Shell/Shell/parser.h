#ifndef PARSER_H_
#define PARSER_H_

#include "list.h"

typedef struct
{
	List args;
} Parser;

extern void initializeParser(Parser* parser);

#endif
