#ifndef LIST_H_
#define LIST_H_

/*************************************************************************************
* 설명: data를 저장하는 변수와 다른 Node를 가리키는 next로 이루어진 구조체
* data: 임의의 data를 저장한다.
* next: 다른 Node를 가리킨다.
*************************************************************************************/
typedef struct Node_ {
	void* data;
	struct Node_* next;
} Node;

/*************************************************************************************
* 설명: Node를 차례로 연결하여 만들어진 linked list를 나타내기 위한 구조체
* head: list의 첫번째 Node를 가리킨다. list가 비어있으면 NULL
* tail: list의 마지막 Node를 가리킨다. list가 비어있으면 NULL
*************************************************************************************/
typedef struct List_ {
	Node* head;
	Node* tail;
	int size;
} List;

extern void initializeList(List* list);
extern void addList(List* list, void* data);
extern void clearList(List* list);
extern void foreachList(List* list, void* aux, void(*action)(void*, void*));

#endif