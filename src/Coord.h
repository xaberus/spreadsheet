#ifndef COORD_H
#define COORD_H

#include "Object.h"

typedef struct {
	Object object;
	int col;
	int row;
} Coord;

extern ObjectVTable CoordVTable;

APIUSE Coord * newCoord(Collector * c, int col, int row);
int coordIsValid(Coord * coord);
unsigned coordURow(Coord * coord);
char coordUCol(Coord * coord);
APIUSE char * coordName(Coord * s);


#endif /* COORD_H */
