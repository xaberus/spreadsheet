#ifndef CELL_H
#define CELL_H

#include "Attribute.h"
#include "Object.h"
#include "Coord.h"
#include "List.h"

typedef struct {
	Coord coord;
	Object * formula;
	Object * value;
	List/*<Cell>*/ * propagate;
	List/*<Cell>*/ * depends;
} Cell;

extern ObjectVTable CellVTable;

APIUSE Cell * newCell(Collector * c, int col, int row);

#endif /* CELL_H */
