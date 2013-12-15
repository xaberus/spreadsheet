#ifndef RANGE_H
#define RANGE_H

#include "Object.h"
#include "Coord.h"

typedef struct {
	Object object;
	Coord * start;
	Coord * end;
} Range;

extern ObjectVTable RangeVTable;

APIUSE Range * newRange(Collector * c, Coord * start, Coord * end);


#endif /* RANGE_H */
