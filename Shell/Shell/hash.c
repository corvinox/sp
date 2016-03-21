#include "hash.h"
#include <stdlib.h>

/*************************************************************************************
* 설명: hash table에 대한 초기화를 수행한다.
* 인자:
* - hash: hash table에 대한 정보를 담고 있는 구조체에 대한 포인터
* - hash_func: key에 대한 hash값을 계산하기 위한 함수를 가리키는 함수포인터
* - cmp: key값을 찾을 때, key 끼리 비교 하기 위한 비교 함수를 가리키는 함수포인터
* 반환값: 없음
*************************************************************************************/
void initializeHash(HashTable* hash, int(*hash_func)(void*), int(*cmp)(void*, void*))
{
	int i;
	for (i = 0; i < BUCKET_SIZE; i++)
		initializeList(&hash->buckets[i]);
	hash->hash_func = hash_func;
	hash->cmp = cmp;
}

/*************************************************************************************
* 설명: hash table에 새로운 entry를 추가한다.
* 인자:
* - hash: hash table에 대한 정보를 담고 있는 구조체에 대한 포인터
* - key: hash table의 entry는 key-value 쌍으로 이루어진다. key
* - value: hash table의 entry는 key-value 쌍으로 이루어진다. value
* 반환값: 없음
*************************************************************************************/
void insertHash(HashTable* hash, void* key, void* value)
{
	Entry* new_entry = (Entry*)malloc(sizeof(Entry));
	new_entry->key = key;
	new_entry->value = value;

	int hash_key = hash->hash_func(key);
	addList(&hash->buckets[hash_key], new_entry);
}

/*************************************************************************************
* 설명: hash table에서 인자로 넘겨받은 key에 해당하는 value 값을 찾는다.
*       초기화에서 등록한 해쉬 함수와 비교함수를 이용하여 같은 key를 갖는 entry를
*       찾고, 그 entry가 갖고있는 value를 반환한다.
* 인자:
* - hash: hash table에 대한 정보를 담고 있는 구조체에 대한 포인터
* - key: hash table에 저장된 entry 중에 같은 key를 찾기 위한 key
* 반환값: 해당 key를 가진 entry의 value
*************************************************************************************/
void* getValue(HashTable* hash, void* key)
{
	int hash_key = hash->hash_func(key);
	List* list = &hash->buckets[hash_key];

	Node* ptr = list->head;
	while (ptr != NULL) {
		Entry* entry = (Entry*)ptr->data;
		if (entry != NULL && !hash->cmp(entry->key, key)) {
			return entry->value;
		}
		ptr = ptr->next;
	}
	
	return NULL;
}

/*************************************************************************************
* 설명: hash table의 모든 entry를 해제한다.
* 인자:
* - hash: hash table에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void clearHash(HashTable* hash)
{
	int i;
	for (i = 0; i < BUCKET_SIZE; i++) {
		clearList(&hash->buckets[i]);
	}
}

/*************************************************************************************
* 설명: hash table의 모든 entry들에 대해서 인자로 받은 action 함수를 실행한다.
* 인자:
* - hash: hash table에 대한 정보를 담고 있는 구조체에 대한 포인터
* - action: entry 이용하여 작업을 수행할 함수를 가리키는 함수포인터
* 반환값: 없음
*************************************************************************************/
void foreachHash(HashTable* hash, void* aux, void(*action)(void*, void*))
{
	int i;
	for (i = 0; i < BUCKET_SIZE; i++) {
		foreachList(&hash->buckets[i], aux, action);
	}
}

