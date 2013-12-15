#include "Object.h"

#include <string.h>

void objectFree(Object * obj) {
	ObjectVTable * vt = obj->vt;
	while (vt) {
		if (vt->free) {
			vt->free(obj);
		}
		vt = vt->parent;
	}
	free(obj);
}

char * objectToString(Object * obj) {
	if (obj) {
		ObjectVTable * vt = obj->vt;
		return vt->toString(obj);
	} else {
		return strdup("nil");
	}
}

int objectInstanceOf(Object * obj, const ObjectVTable * vt) {
	if (!obj) return 0;
	return obj->vt == vt;
}

const char * objectTypeName(Object * obj) {
	return obj->vt->name;
}
