#include "Error.h"

#include <string.h>

static char * errorToString(Object * obj) {
	Error * s = (Error *) obj;
	return strdup(s->value);
}

static void errorFree(Object * obj) {
	Error * s = (Error *) obj;
	free(s->value);
}

ObjectVTable ErrorVTable = {
	.name = "Error",

	.parent = NULL,
	.toString = errorToString,
	.free = errorFree,
};

Error * newError(Collector * c, const char * string) {
	Error * result = malloc(sizeof(Error));
	if (!result) return NULL;
	((Object *) result)->vt = &ErrorVTable;
	result->value = strdup(string);
	if (!result->value) goto fail;
	if (!collectorIntern(c, (Object *) result)) goto fail;
	return result;
fail:
	if (result->value) free(result->value);
	if (result) free(result);
	return NULL;
}
