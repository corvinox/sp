#include "loader.h"
#include "mytype.h"


BOOL loaderInitialize(Loader* loader, SICXEVM* sicxevm)
{
	if (!loader)
		return false;
	loader->initialized = false;
	loader->sicxevm = sicxevm;
	loader->prog_addr = 0;

	/* intiailize ¼º°ø */
	loader->initialized = true;
	return true;
}

void loaderRelease(Loader* loader)
{
	if (!loader)
		return;

	loader->initialized = false;
	loader->sicxevm = NULL;
	loader->prog_addr = 0;
	loaderClearBreakPoints(loader);
}

BOOL loaderIsInitialized(Loader* loader)
{
	return loader && loader->initialized;
}

void loaderSetProgAddr(Loader* loader, WORD prog_addr)
{
	if (!loader || !loader->initialized)
		return;

	loader->prog_addr = prog_addr;
}

void loaderLinkLoad(Loader* loader)
{
	if (!loader || !loader->initialized)
		return;
}

void loaderSetBreakPoint(Loader* loader, WORD addr)
{
	listAdd(&loader->break_points, (void*)addr);
}

void loaderClearBreakPoints(Loader* loader)
{
	listClear(&loader->break_points);
}