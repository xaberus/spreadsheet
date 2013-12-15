#ifndef TABLE_H
#define TABLE_H

#include "Object.h"
#include "Cell.h"
#include "Buffer.h"
#include "Error.h"

typedef struct Table {
	Object object;
	int cols;
	int rows;
	Cell ** cells;
	Collector * c;

	// errors
	Error * ecall;
	Error * eerror;
	Error * eformula;
	Error * elink;
	Error * eloop;
	Error * erefs;
	Error * eundefined;

} Table;

extern ObjectVTable TableVTable;

APIUSE Table * newTable(Collector * c, int cols, int rows);
APIUSE Cell * tableGet(Table * table, int col, int row);
void tableClearCell(Table * table, Cell * cell);
Object * tableEvaluateCell(Table * table, Cell * target, Cell * cell);
Object * tableEvaluateFormula(Table * table, Cell * target, Object * obj);
void tableClear(Table * table);
APIUSE int tableResize(Table * table, int cols, int rows);

APIUSE int tableSetCell(Table * table, Cell * cell, Buffer * value);
APIUSE int tableImport(Table * table, const char * file);
APIUSE int tableExport(Table * table, const char * file);

#endif /* TABLE_H */
