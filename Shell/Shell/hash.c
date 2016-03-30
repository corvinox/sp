#include "hash.h"
#include <stdlib.h>

/*************************************************************************************
* 설명: hash table에 대한 초기화를 수행한다.
* 인자:
* - hash: hash table에 대한 정보를 담고 있는 구조체에 대한 포인터
* - bucket_size: hash table이 갖는 bucket의 크기
* - hash_func: key에 대한 hash값을 계산하기 위한 함수를 가리키는 함수포인터
* - cmp: key값을 찾을 때, key 끼리 비교 하기 위한 비교 함수를 가리키는 함수포인터
* 반환값: 없음
*************************************************************************************/
void hashInitialize(HashTable* hash, int bucket_size, int(*hash_func)(void*), int(*cmp)(void*, void*))
{
	hash->bucket_size = bucket_size;
	hash->hash_func = hash_func;
	hash->cmp = cmp;
	hash->size = 0;

	hash->buckets = (List*)calloc(sizeof(List), bucket_size);
	for (int i = 0; i < bucket_size; i++)
		listInitialize(hash->buckets + i);
}

/*************************************************************************************
* 설명: hash table을 초기화 할 때, 할당했던 메모리를 모두 해제한다. 추가된 entry에 
*       대한 메모리 해제는 일어나지 않으므로 외부에서 반드시 entry들에 대해
*       할당했던 메모리를 해주어야만 한다.
* 인자:
* - hash: hash table에 대한 정보를 담고 있는 구조체에 대한 포인터
* - hash_func: key에 대한 hash값을 계산하기 위한 함수를 가리키는 함수포인터
* - cmp: key값을 찾을 때, key 끼리 비교 하기 위한 비교 함수를 가리키는 함수포인터
* 반환값: 없음
*************************************************************************************/
void hashRelease(HashTable* hash)
{
	hashClear(hash);
	free(hash->buckets);
}

/*************************************************************************************
* 설명: hash table에 새로운 entry를 추가한다.
* 인자:
* - hash: hash table에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void hashInsert(HashTable* hash, void* key, void* value)
{
	HashEntry* new_entry = (HashEntry*)malloc(sizeof(HashEntry));
	new_entry->key = key;
	new_entry->value = value;

	int hash_key = hash->hash_func(key);
	listAdd(&hash->buckets[hash_key], new_entry);

	hash->size++;
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
void* hashGetValue(HashTable* hash, void* key)
{
	int hash_key = hash->hash_func(key);
	List* list = &hash->buckets[hash_key];
		
	for (Node* ptr = list->head; ptr != NULL;  ptr = ptr->next) {
		HashEntry* entry = (HashEntry*)ptr->data;
		if (entry != NULL && !hash->cmp(entry->key, key)) {
			return entry->value;
		}
	}
	
	return NULL;
}

/*************************************************************************************
* 설명: hash table의 모든 entry를 해제한다.
* 인자:
* - hash: hash table에 대한 정보를 담고 있는 구조체에 대한 포인터
* 반환값: 없음
*************************************************************************************/
void hashClear(HashTable* hash)
{
	for (int i = 0; i < hash->bucket_size; i++)
		listClear(&hash->buckets[i]);
	hash->size = 0;
}

/*************************************************************************************
* 설명: hash table의 모든 entry들에 대해서 인자로 받은 action 함수를 실행한다.
* 인자:
* - hash: hash table에 대한 정보를 담고 있는 구조체에 대한 포인터
* - action: entry 이용하여 작업을 수행할 함수를 가리키는 함수포인터
* 반환값: 없음
*************************************************************************************/
void hashForeach(HashTable* hash, void* aux, void(*action)(void*, void*))
{
	for (int i = 0; i < hash->bucket_size; i++)
		listForeach(&hash->buckets[i], aux, action);
}

