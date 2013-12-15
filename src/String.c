#include "String.h"

#include <string.h>

static char * stringToString(Object * obj) {
	String * s = (String *) obj;
	return strdup(s->value);
}

static void stringFree(Object * obj) {
	String * s = (String *) obj;
	free(s->value);
}

ObjectVTable StringVTable = {
	.name = "String",

	.parent = NULL,
	.toString = stringToString,
	.free = stringFree,
};

String * newString(Collector * c, const char * string) {
	return newStringN(c, strlen(string), string);
}

// assumes ownership of string
String * newStringOwning(Collector * c, char * string) {
	if (!string) return NULL;
	String * result = malloc(sizeof(String));
	if (!result) return NULL;
	((Object *) result)->vt = &StringVTable;
	result->value = string;
	if (!collectorIntern(c, (Object *) result)) goto fail;
	return result;
fail:
	if (result) free(result);
	if (string) free(string);
	return NULL;
}

String * newStringN(Collector * c, int len, const char string[len]) {
	String * result = malloc(sizeof(String));
	if (!result) return NULL;
	((Object *) result)->vt = &StringVTable;
	result->value = strndup(string, len);
	if (!result->value) goto fail;
	if (!collectorIntern(c, (Object *) result)) goto fail;
	return result;
fail:
	if (result->value) free(result->value);
	if (result) free(result);
	return NULL;
}
