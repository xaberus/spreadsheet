#include "List.h"

#include <string.h>
#include <assert.h>

ListNode * newListNode(Object * obj) {
	ListNode * result = malloc(sizeof(ListNode));
	if (!result) return NULL;
	result->value = obj;
	// delete me?
	result->prev = NULL;
	result->next = NULL;
	return result;
}

int listLength(List * list) {
	int c = 0;
	ListNode * n = list->first;
	while (n) {
		c++;
		n = n->next;
	}
	return c;
}

Object * listGet(List * list, int i) {
	ListNode * node = NULL;
	if (i >= 0) {
		node = list->first;
		while (node && i-- > 0) node = node->next;
	} else {
		node = list->last;
		while (node && ++i < 0) node = node->prev;
	}
	if (!node) return NULL;
	return node->value;
}

int listAppend(List * list, Object * obj) {
	ListNode * node = newListNode(obj);
	if (!node) {
		if (list->ownership) objectFree(obj);
		return 0;
	}

	if (!list->last) {
		node->next = NULL;
		node->prev = NULL;
		list->first = list->last = node;
	} else {
		node->next = NULL;
		node->prev = list->last;
		node->prev->next = node;
		list->last = node;
	}

	return 1;
}

// todo: cleanup mess
static char * listToString(Object * obj) {
	List * list = (List * ) obj;
	int n = listLength(list);
	char ** strings = malloc(n * sizeof(char *));
	int i, totalLength = 3; // '(', ')', '\0'
	for (i = 0; i < n; i++) {
		strings[i] = objectToString(listGet(list, i));
		totalLength += strlen(strings[i]) + 2; // string + ",_"
	}
	char *buffer = calloc(totalLength, 1);
	buffer[0] = '(';
	int j = 1;
	for (i = 0; i < n; i++) {
		strcpy(buffer + j, strings[i]);
		j += strlen(strings[i]);
		if (i < n-1) {
			strcpy(buffer + j, ", ");
			j += 2;
		}
	}
	for (i = 0; i < n; i++) free(strings[i]);
	free(strings);
	buffer[j++] = ')';
	buffer[j++] = '\0';
	return buffer;
}

/*
 * unlinks and removes a node from list, returning the value
 * expects valid node!
 */
static Object * listUnlink(List * list, ListNode * node) {
	Object * value = node->value;
	ListNode * first = list->first;
	ListNode * last = list->last;
	if (first == last) {
		list->first = NULL;
		list->last = NULL;
	} else {
		// at least 2 nodes in list!
		if (node == first) {
			list->first = first->next;
			list->first->prev = NULL;
		} else if (node == last) {
			list->last = last->prev;
			list->last->next = NULL;
		} else {
			node->prev->next = node->next;
			node->next->prev = node->prev;
		}
	}
	free(node);
	return value;
}

/*
 * for i > 0 removes i'th node from the lists start and returns its value
 * for i < 0 removes (i-1)'th node from the lists end and returns its value
 * 	(starting to count the end from 0, -1 removes last element)
 * list looses ownership to object!
 */
Object * listPopN(List *list, int i) {
	ListNode * node = NULL;
	if (i >= 0) {
		node = list->first;
		while (node && i-- > 0) node = node->next;
	} else {
		node = list->last;
		while (node && ++i < 0) node = node->prev;
	}
	if (!node) return NULL;

	return listUnlink(list, node);
}

/*
 * removes last element from list
 * list looses ownership to object!
 */
Object * listPop(List *list) {
	ListNode * last = list->last;
	if (!last) return NULL;

	return listUnlink(list, last);
}

void listRemove(List * list, Object * rem) {
	ListNode * node = list->first;
	while (node != NULL) {
		ListNode * next = node->next;
		if (node->value == rem) listUnlink(list, node);
		node = next;
	}
}

void listRemoveAll(List * list) {
	ListNode * c = list->first;
	while (c) {
		ListNode * n = c->next;
		if (list->ownership) objectFree(c->value);
		free(c);
		c = n;
	}
	list->first = list->last = NULL;
}

int listIsEmpty(List * list) {
	return list->first == NULL;
}

/**/

static void listFree(Object * obj) {
	List * list = (List * ) obj;
	listRemoveAll(list);
}

static void listMarkCallback(Object * o, void * ctx) {
	Collector * c = ctx;
	collectorMark(c, o);
}

static void listPropagateMark(Object * obj, Collector * c) {
	List * list = (List * ) obj;
	listForEach(list, listMarkCallback, c);
}

int listContains(List * list, Object * rem) {
	ListNode * node = list->first;
	while (node != NULL) {
		ListNode * next = node->next;
		if (node->value == rem) return 1;
		node = next;
	}
	return 0;
}


ObjectVTable ListVTable = {
	.name = "List",

	.parent = NULL,
	.toString = listToString,
	.free = listFree,
	.propagateMark = listPropagateMark,
};

List * newListBare(int ownership) {
	List * result = malloc(sizeof(List));
	if (!result) return NULL;
	result->object.vt = &ListVTable;
	result->ownership = ownership;
	result->first = NULL;
	result->last = NULL;
	return result;
}

List * newList(Collector * c) {
	List * list = newListBare(0);
	if (!list) return NULL;
	if (!collectorIntern(c, (Object *) list)) goto fail;
	return list;
fail:
	if (list) free(list);
	return NULL;
}

List * listMap(Collector * c, List * list, ListMapFunction fn, void * ctx) {
	List * result = c ? newList(c) : newListBare(1);
	ListNode * node;
	for (node = list->first; node != NULL; node = node->next) {
		Object * o = fn(node->value, ctx); if (!o) goto fail;
		if (!listAppend(result, o)) goto fail;
	}
	return result;
fail:
	if (!c) objectFree((Object *) list);
	return NULL;
}

void listForEach(List * list, ListElementFunction fn, void * ctx) {
	ListNode * node;
	for (node = list->first; node != NULL; node = node->next) {
		fn(node->value, ctx);
	}
}

void listFlatForEach(List * list, ListElementFunction fn, void * ctx) {
	ListNode * node = list->first;
	while(node) {
		Object * value = node->value;
		if (objectInstanceOf(value, &ListVTable)) {
			listFlatForEach((List *) value, fn, ctx);
		} else {
			fn(value, ctx);
		}
		node = node->next;
	}
}

List * listFilter(Collector * c, List * list, ListSelectFunction fn, void * ctx) {
	List * result = c ? newList(c) : newListBare(0);
	ListNode * node;
	for (node = list->first; node != NULL; node = node->next) {
		if (fn(node->value, ctx)) {
			if (!listAppend(result, node->value)) goto fail;
		}
	}
	return result;
fail:
	if (!c)	objectFree((Object *) list);
	return NULL;
}

static int listFlatFilterRec(List * list, List * result, ListSelectFunction fn, void * ctx) {
	ListNode * node;
	for (node = list->first; node != NULL; node = node->next) {
		Object * value = node->value;
		if (objectInstanceOf(value, &ListVTable)) {
			listFlatFilterRec((List *) value, result, fn, ctx);
		} else {
			if (fn(node->value, ctx)) {
				if (!listAppend(result, node->value)) goto fail;
			}
		}
	}
	return 1;
fail:
	return 0;
}

List * listFlatFilter(Collector * c, List * list, ListSelectFunction fn, void * ctx) {
	List * result = c ? newList(c) : newListBare(0);
	if (!listFlatFilterRec(list, result, fn, ctx)) goto fail;
	return result;
fail:
	if (!c)	objectFree((Object *) list);
	return NULL;
}
