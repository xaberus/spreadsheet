#ifndef OBJECT_H
#define OBJECT_H

#include "Collector.h"

#define DEBUG

typedef struct ObjectVTable ObjectVTable;

struct Object {
	struct ObjectVTable * vt;
	unsigned state;
};

struct ObjectVTable {
	struct ObjectVTable * parent;

	const char * name;

	char * (*toString)(Object * o);
	void (*free)(Object * o);
	void (*propagateMark)(Object * o, Collector * c);
};

void objectFree(Object * obj);
APIUSE char * objectToString(Object * obj);
int objectInstanceOf(Object * obj, const ObjectVTable * vt);
const char * objectTypeName(Object * obj);

#endif
