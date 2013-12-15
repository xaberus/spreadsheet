#include "Range.h"

#include <stdio.h>
#include <string.h>

static char * rangeToString(Object * obj) {
	Range * range = (Range * ) obj;
	if (!coordIsValid(range->start) || !coordIsValid(range->end)) return strdup("INVALID");
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%c%u:%c%d",
		range->start->col + 'A', range->start->row + 1,
		range->end->col + 'A', range->end->row + 1);
	return strdup(buffer);
}

static void rangePropagateMark(Object * obj, Collector * c) {
	Range * range = (Range * ) obj;
	collectorMark(c, (Object *) range->start);
	collectorMark(c, (Object *) range->end);
}

ObjectVTable RangeVTable = {
	.name = "Range",

	.parent = NULL,
	.toString = rangeToString,
	.free = NULL,
	.propagateMark = rangePropagateMark,
};

Range * newRange(Collector * c, Coord * start, Coord * end) {
	Range * result = malloc(sizeof(Range));
	if (!result) return NULL;
	((Object *) result)->vt = &RangeVTable;
	result->start = start;
	result->end = end;
	if (!collectorIntern(c, (Object *) result)) goto fail;
	return result;
fail:
	free(result);
	return NULL;
}

