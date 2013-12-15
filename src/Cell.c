#include "Cell.h"

#include "Object.h"
#include "Double.h"
#include "String.h"
#include "List.h"

#include <stdio.h>
#include <string.h>

extern ObjectVTable CoordVTable;


static char * cellToString(Object * obj) {
	return CoordVTable.toString(obj);
}

static void cellPropagateMark(Object * obj, Collector * c) {
	Cell * cell = (Cell *) obj;
	if (cell->formula) collectorMark(c, (Object *) cell->formula);
	if (cell->value) collectorMark(c, (Object *) cell->value);
	collectorMark(c, (Object *) cell->propagate);
	collectorMark(c, (Object *) cell->depends);
}

ObjectVTable CellVTable = {
	.name = "Cell",

	.parent = &CoordVTable,
	.toString = cellToString,
	.free = NULL,
	.propagateMark = cellPropagateMark,
};

Cell * newCell(Collector * c, int col, int row) {
	Cell * result = NULL;
	List * propagate = NULL;
	List * depends = NULL;

	result = malloc(sizeof(Cell));
	if (!result) return NULL;
	propagate = newList(c);
	if (!propagate) goto fail;
	depends = newList(c);
	if (!depends) goto fail;

	((Object *) result)->vt = &CellVTable;
	((Coord *) result)->col = col;
	((Coord *) result)->row = row;
	result->formula = NULL;
	result->value = NULL;
	result->propagate = propagate;
	result->depends = depends;
	if (!collectorIntern(c, (Object *) result)) goto fail;
	return result;
fail:
	if (result) free(result);
	if (propagate) objectFree((Object *) propagate);
	if (depends) objectFree((Object *) depends);
	return NULL;
}

