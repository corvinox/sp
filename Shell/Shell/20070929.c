#include "sicxevm.h"
#include "shell.h"
#include "assembler.h"
#include "loader.h"


int main() 
{ 
	SICXEVM sicxevm;
	Assembler asmblr;
	Loader loader;
	Shell shell;
	
	/* 초기화 */
	vmInitialize(&sicxevm);
	assemblerInitialize(&asmblr, &sicxevm);
	loaderInitialize(&loader, &sicxevm);
	shellInitialize(&shell, &sicxevm, &asmblr, &loader);

	/* shell 실행 */
	shellStart(&shell);

	/* shell이 종료되면 사용된 모든 자원을 해제 */
	shellRelease(&shell);
	loaderRelease(&loader);
	assemblerRelease(&asmblr);
	vmRelease(&sicxevm);

	return 0;
}