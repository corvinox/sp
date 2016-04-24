#ifndef LOADER_H_
#define LOADER_H_

#include "mytype.h"
#include "list.h"
#include "sicxevm.h"

typedef struct _Loader
{
	BOOL initialized;
	SICXEVM* sicxevm;
	WORD prog_addr;
	List break_points;
} Loader;

extern BOOL loaderInitialize(Loader* loader, SICXEVM* sicxevm);
extern BOOL loaderIsInitialized(Loader* loader);
extern void loaderRelease(Loader* loader);
extern void loaderSetProgAddr(Loader* loader, WORD prog_addr);
extern void loaderLinkLoad(Loader* loader);
extern void loaderSetBreakPoint(Loader* loader, WORD addr);
extern void loaderClearBreakPoints(Loader* loader);

#endif