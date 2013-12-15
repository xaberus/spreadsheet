#include "Coord.h"

#include <stdio.h>
#include <string.h>

char * coordName(Coord * c) {
	if (!coordIsValid(c)) return strdup("#INVALID");

	char buffer[1 + 10 + 1]; // "<column><int32_10>\0";
	snprintf(buffer, sizeof(buffer), "%c%u", coordUCol(c), coordURow(c));
	return strdup(buffer);
}

static char * coordToString(Object * obj) {
	return coordName((Coord * ) obj);
}

int coordIsValid(Coord * coord) {
	return (coord->col < 26 && coord->col >= 0 && coord->row >= 0);
}

unsigned coordURow(Coord * coord) {
	return coordIsValid(coord) ? coord->row + 1 : -1;
}

char coordUCol(Coord * coord) {
	return coordIsValid(coord) ? coord->col + 'A' : '#';
}

ObjectVTable CoordVTable = {
	.name = "Coord",

	.parent = NULL,
	.toString = coordToString,
	.free = NULL,
	.propagateMark = NULL,
};

Coord * newCoord(Collector * c, int col, int row) {
	Coord * result = malloc(sizeof(Coord));
	if (!result) return NULL;
	result->object.vt = &CoordVTable;
	result->col = col;
	result->row = row;
	if (!collectorIntern(c, (Object *) result)) goto fail;
	return result;
fail:
	free(result);
	return NULL;
}
