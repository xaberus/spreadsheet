#include "Call.h"

#include <string.h>

static char * callToString(Object * obj) {
	Call * call = (Call *) obj;
	char * fn = objectToString((Object *) call->func);
	if (!fn) return NULL;
	char * args = objectToString((Object *) call->args);
	if (!args) {
		free(fn);
		return NULL;
	}

	size_t lfn = strlen(fn);
	size_t largs = strlen(args);

	char * buffer = malloc(lfn + largs + 1);
	if (!buffer) {
		free(fn);
		free(args);
		return NULL;
	}

	memcpy(buffer, fn, lfn);
	memcpy(buffer + lfn, args, largs);
	buffer[lfn + largs] = '\0';

	free(fn);
	free(args);

	return buffer;
}

static void cellPropagateMark(Object * obj, Collector * c) {
	Call * call = (Call *) obj;
	collectorMark(c, (Object *) call->func);
	collectorMark(c, (Object *) call->args);
}

ObjectVTable CallVTable = {
	.name = "Call",

	.parent = NULL,
	.toString = callToString,
	.free = NULL,
	.propagateMark = cellPropagateMark,
};

Call * newCall(Collector * c, String * func, List * args) {
	Call * result = malloc(sizeof(Call));
	if (!result) return NULL;
	result->object.vt = &CallVTable;
	result->func = func;
	result->args = args;
	if (!collectorIntern(c, (Object *) result)) goto fail;
	return result;
fail:
	free(result);
	return NULL;
}
