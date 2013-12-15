#ifndef PATTERN_H
#define PATTERN_H

#include "Object.h"
#include "Table.h"

typedef struct {
	Object object;
	int dcol;
	int drow;
} Pattern;

extern ObjectVTable PatternVTable;

APIUSE Pattern * newPattern(Collector * c, int col, int row);
APIUSE Cell * patternGet(Pattern * pattern, Table * table, Cell * cell);


#endif /* PATTERN_H */
