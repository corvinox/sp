#include "shell.h"

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
