#include "Collector.h"
#include "List.h"

#include <assert.h>

struct Collector {
	List * roots;
	List * objects;
	List * grays;
};

typedef enum {
	GRAY = 0,
	WHITE = 1,
	BLACK = 2,
} ObjectState;

void collectorMarkWhite(Object * o, void * ctx) {
	(void) ctx;
	o->state = WHITE;
}

void collectorMarkGray(Object * o, void * ctx) {
	Collector * c = ctx;
	o->state = GRAY;
	assert(listAppend(c->grays, o));
}

void collectorDeleteWhite(Object * o, void * ctx) {
	(void) ctx;
	assert(o->state != GRAY);
	if (o->state == WHITE) objectFree(o);
}

void collectorRun(Collector * c) {
	// mark all objects white
	listForEach(c->objects, collectorMarkWhite, NULL);
	listForEach(c->roots, collectorMarkGray, c);

	// let all objects mark their links
	while (!listIsEmpty(c->grays)) {
		Object * o = listPop(c->grays);
		o->state = BLACK;
		if (o->vt->propagateMark) {
			o->vt->propagateMark(o, c);
		}
	}

	assert(listIsEmpty(c->grays));

	// collect unreachable objects
	ListNode * node = c->objects->first, * next;
	while (node) {
		next = node->next;
		Object * o = node->value;
		assert(o->state != GRAY);
		if (o->state == WHITE) {
			listRemove(c->objects, o);
			objectFree(o);
		}
		node = next;
	}
	listForEach(c->objects, collectorDeleteWhite, NULL);
}

Collector * newCollector() {
	Collector * c = malloc(sizeof(Collector));
	if (!c) return NULL;

	List * objects = NULL;
	List * grays = NULL;
	List * roots = NULL;

	objects = newListBare(1);
	if (!objects) goto fail;
	grays = newListBare(0);
	if (!grays) goto fail;
	roots = newListBare(0);
	if (!roots) goto fail;

	c->objects = objects;
	c->roots = roots;
	c->grays = grays;

	return c;
fail:
	if (c) free(c);
	if (objects) free(objects);
	if (grays) free(grays);
	if (roots) free(roots);

	return NULL;
}

int collectorIntern(Collector * c, Object * o) {
	return listAppend(c->objects, o);
}

int collectorRoot(Collector * c, Object * o) {
	return listAppend(c->roots, o);
}


void collectorFree(Collector * c) {
	objectFree((Object *) c->roots);
	objectFree((Object *) c->grays);
	objectFree((Object *) c->objects);
	free(c);
}

void collectorMark(Collector * c, Object * o) {
	if (!o) return;
	if (o->state == WHITE) {
		if (o->vt->propagateMark) {
			o->state = GRAY;
			assert(listAppend(c->grays, o));
		} else {
			o->state = BLACK;
		}
	}
}
