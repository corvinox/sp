#ifndef HASH_H_
#define HASH_H_

#include "list.h"

#define BUCKET_SIZE 20

/*************************************************************************************
* 설명: key와 value를 쌍으로 저장하기 위한 구조체.
* key: key
* value: value
*************************************************************************************/
typedef struct {
	void* key;
	void* value;
} Entry;

/*************************************************************************************
* 설명: key-value entry를 data로 갖는 linked-list들의 배열로 구성된 hash table.
*       사용하기 전에 bucket에 들어있는 list를 초기화하고, hash_func과 cmp 함수
*       를 등록하기 위해 반드시 initializeHash를 실행해야 한다.
* buckets: 각 해쉬값에 해당하는 list를 갖고 있는 배열
* hash_func: key에 대한 hash값을 계산하기 위한 함수를 가리키는 함수포인터
* cmp: key값을 찾을 때, key 끼리 비교 하기 위한 비교 함수를 가리키는 함수포인터
*************************************************************************************/
typedef struct {
	List buckets[BUCKET_SIZE];
	int(*hash_func)(void*);
	int(*cmp)(void*, void*);
} HashTable;

extern void hashInitialize(HashTable* hash, int(*hash_func)(void*), int(*cmp)(void*, void*));
extern void hashInsert(HashTable* hash, void* key, void* value);
extern void hashClear(HashTable* hash);
extern void hashForeach(HashTable* hash, void* aux, void(*action)(void*, void*));
extern void* hashGetValue(HashTable* hash, void* key);

#endif