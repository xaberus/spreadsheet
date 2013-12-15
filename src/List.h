#ifndef LIST_H
#define LIST_H

#include "Attribute.h"
#include "Object.h"

typedef struct ListNode {
	Object * value;
	struct ListNode *next;
	struct ListNode *prev;
} ListNode;

extern ObjectVTable ListNodeVTable;

typedef struct {
	Object object;
	int ownership;
	ListNode * first;
	ListNode * last;
} List;

extern ObjectVTable ListVTable;

List * newList(Collector * c);
List * newListBare(int ownership);

int listLength(List * list);
Object * listGet(List * list, int i);
APIUSE int listAppend(List * list, Object * obj);
Object * listPopN(List *list, int i);
Object * listPop(List *list);
void listRemove(List * list, Object * rem);
void listRemoveAll(List * list);
int listIsEmpty(List * list);
int listContains(List * list, Object * rem);

typedef Object * (*ListMapFunction)(Object *, void *);
typedef void (*ListElementFunction)(Object *, void *);
typedef int (*ListSelectFunction)(Object *, void *);

List * listMap(Collector * c, List * list, ListMapFunction fn, void * ctx);
void listForEach(List * list, ListElementFunction fn, void * ctx);
void listFlatForEach(List * list, ListElementFunction fn, void * ctx);
List * listFilter(Collector * c, List * list, ListSelectFunction fn, void * ctx);
List * listFlatFilter(Collector * c, List * list, ListSelectFunction fn, void * ctx);


#endif /* LIST_H */
