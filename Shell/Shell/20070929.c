#include "shell.h"

int main() 
{ 
	Shell shell;
	
	/* 초기화 */
	shellInitialize(&shell);

	/* shell 실행 */
	shellStart(&shell);

	/* shell이 종료되면 사용된 모든 자원을 해제 */
	shellRelease(&shell);

	return 0;
}