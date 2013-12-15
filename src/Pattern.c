#include "Pattern.h"

#include <string.h>

static char * patternToString(Object * obj) {
	Pattern * pattern = (Pattern *) obj;

	char buf[64];
	snprintf(buf, sizeof(buf), "[%d,%d]", pattern->dcol, pattern->drow);
	return strdup(buf);
}

ObjectVTable PatternVTable = {
	.name = "Pattern",

	.parent = NULL,
	.toString = patternToString,
	.free = NULL,
	.propagateMark = NULL,
};

Pattern * newPattern(Collector * c, int dcol, int drow) {
	Pattern * result = malloc(sizeof(Pattern));
	if (!result) return NULL;
	result->object.vt = &PatternVTable;
	result->dcol = dcol;
	result->drow = drow;
	if (!collectorIntern(c, (Object *) result)) goto fail;
	return result;
fail:
	free(result);
	return NULL;
}

Cell * patternGet(Pattern * pattern, Table * table, Cell * cell) {
	int col = cell->coord.col + pattern->dcol;
	int row = cell->coord.row + pattern->drow;

	return tableGet(table, col, row);
}
