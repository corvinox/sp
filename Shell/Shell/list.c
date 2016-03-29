#include "list.h"
#include <stdio.h>
#include <stdlib.h>

/*************************************************************************************
* 설명: list에 대한 초기화를 수행한다.
* 인자:
* - list: list에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void listInitialize(List* list)
{
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}

/*************************************************************************************
* 설명: list의 끝에 새로운 node를 추가한다.
* 인자:
* - list: list에 대한 정보를 담고 있는 구조체에 대한 포인터
* - data: data를 가리키는 포인터
* 반환값: 없음
*************************************************************************************/
void listAdd(List* list, void* data)
{
	Node* new_node = (Node*)malloc(sizeof(Node));
	new_node->data = data;
	new_node->next = NULL;

	if (list->head == NULL) {
		list->head = new_node;
		list->tail = new_node;
	}
	else {
		list->tail->next = new_node;
		list->tail = new_node;
	}
	
	list->size++;
}

/*************************************************************************************
* 설명: list의 모든 node를 해제한다.
* 인자:
* - list: list에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void listClear(List* list)
{
	Node* ptr = list->head;
	while (ptr != NULL) {
		Node* next = ptr->next;
		free(ptr);
		ptr = next;
	}

	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
}

/*************************************************************************************
* 설명: list의 모든 node들의 data에 대해서 인자로 받은 action 함수를 실행한다.
* 인자:
* - list: list에 대한 정보를 담고 있는 구조체에 대한 포인터
* - action: data 이용하여 작업을 수행할 함수에 대한 함수포인터
* 반환값: 없음
*************************************************************************************/
void listForeach(List* list, void* aux, void(*action)(void*, void*))
{
	if (list->head == NULL)
		return;

	Node* ptr = list->head;
	while (ptr != NULL) {
		action(ptr->data, aux);
		ptr = ptr->next;
	}
}