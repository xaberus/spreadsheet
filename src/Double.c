#include "Double.h"

#include <stdio.h>
#include <string.h>

static char * doubleToString(Object * obj) {
	Double * d = (Double * ) obj;
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%g", d->value);
	return strdup(buffer);
}

ObjectVTable DoubleVTable = {
	.name = "Double",
	
	.parent = NULL,
	.toString = doubleToString,
	.free = NULL,
};

Double * newDouble(Collector *c, double d) {
	Double * result = malloc(sizeof(Double));
	if (!result) return NULL;
	result->object.vt = &DoubleVTable;
	result->value = d;
	if (!collectorIntern(c, (Object *) result)) goto fail;
	return result;
fail:
	free(result);
	return NULL;
}
