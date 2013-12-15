#include "Table.h"
#include "List.h"
#include "Double.h"
#include "String.h"
#include "Call.h"
#include "Range.h"
#include "Pattern.h"

#include "Formula.h"

#include "TableDispatch.h"

#include <string.h>
#include <stdio.h>


#define RED(s) "\x1b[1;31m" s "\x1b[0m"
#define YELLOW(s) "\x1b[1;33m" s "\x1b[0m"
#define GREEN(s) "\x1b[1;32m" s "\x1b[0m"
#define BLUE(s) "\x1b[1;34m" s "\x1b[0m"
#define PINK(s) "\x1b[1;35m" s "\x1b[0m"

//#define debug(...)

#ifndef debug

#include "DebugPrintf.h"

#define debug(L, ...) debug_ ## L(__VA_ARGS__)

#define debug_0(...) debugPrintf(stdout, "(0) " __VA_ARGS__)
#define debug_1(...) debugPrintf(stdout, "(1) " __VA_ARGS__)
#define debug_2(...) //debugPrintf(stdout, "(2) " __VA_ARGS__)
#define debug_3(...) //debugPrintf(stdout, "(3) " __VA_ARGS__)
#define debug_4(...) //debugPrintf(stdout, "(4) " __VA_ARGS__)

#endif

void tableFree(Object * obj) {
	Table * t = (Table *) obj;
	free(t->cells);
}

static void tablePropagateMark(Object * obj, Collector * c) {
	Table * t = (Table *) obj;
	int ncells = t->cols * t->rows;
	for (int i = 0; i < ncells; i++) {
		collectorMark(c, (Object *) t->cells[i]);
	}

	collectorMark(c, (Object *) t->ecall);
	collectorMark(c, (Object *) t->eerror);
	collectorMark(c, (Object *) t->eformula);
	collectorMark(c, (Object *) t->elink);
	collectorMark(c, (Object *) t->eloop);
	collectorMark(c, (Object *) t->erefs);
	collectorMark(c, (Object *) t->eundefined);
}

Cell * tableGet(Table * table, int col, int row) {
	if (col < 0 || col >= table->cols) return NULL;
	if (row < 0 || row >= table->rows) return NULL;

	return *(table->cells + (row * table->cols + col));
}

inline static int objectIsValue(Object * obj) {
	if (objectInstanceOf(obj, &DoubleVTable)) {
		return 1;
	} else if (objectInstanceOf(obj, &StringVTable)) {
		return 1;
	}
	return 0;
}

int tableCellReferences(Table * table, Cell * cell, List * refs) {
	(void) table;
	if (!listContains(refs, (Object *) cell)) {
		return listAppend(refs, (Object *) cell);
	}
	return 1;
}

int tableRangeReferences(Table * table, Range * rng, List * refs) {
	debug(2, "%f(rng = %p, refs = %p)\n", __FUNCTION__, rng, refs);
	for (int r = rng->start->row, re = rng->end->row; r <= re; r++) {
		for (int c = rng->start->col, ce = rng->end->col; c <= ce; c++) {
			Cell * ref = tableGet(table, c, r);
			if (ref) {
				if (!tableCellReferences(table, ref, refs)) return 0;;
			}
		}
	}
	return 1;
}

int tableFormulaReferences(Table * table, Object * obj, List * refs);

int tableCallReferences(Table * table, Call * call, List * refs) {
	debug(2, "%f(call = %p)\n", __FUNCTION__, call);
	List * args = call->args;

	// collect
	ListNode * node = args->first;
	while (node) {
		if (objectInstanceOf(node->value, &RangeVTable)) {
			Range * rng = (Range *) node->value;
			if (!tableRangeReferences(table, rng, refs)) return 0;;
		} else {
			if (!tableFormulaReferences(table, node->value, refs)) return 0;
		}
		node = node->next;
	}

	return 1;
}

int tableFormulaReferences(Table * table, Object * obj, List * refs) {
	debug(2, "%f(obj = %p)\n", __FUNCTION__, obj);
	if (objectInstanceOf(obj, &CoordVTable)) {
		Coord * coord = (Coord *) obj;
		Cell * ref = tableGet(table, coord->col, coord->row);
		if (ref) {
			return tableCellReferences(table, ref, refs);
		}
	} else if (objectInstanceOf(obj, &RangeVTable)) {
		Range * rng = (Range *) obj;
		return tableRangeReferences(table, rng, refs);
	} else if (objectInstanceOf(obj, &CallVTable)) {
		Call * call = (Call *) obj;
		return tableCallReferences(table, call, refs);
	}
	return 1;
}

Object * tableEvaluateCell(Table * table, Cell * target, Cell * cell) {
	debug(3, "%f(target = %p, cell = %p (dep: %p prop: %p))\n",
		__FUNCTION__, target, cell, cell->depends, cell->propagate);
	if (cell->value) return cell->value;
	if (!cell->formula) return NULL;

	if (listContains(cell->depends, (Object *) cell)) {
		debug(1, "# loop detected for %p through %p\n", target, cell);
		cell->value = (Object *) table->eloop;
	} else {
		if (objectIsValue(cell->formula)) {
			cell->value = cell->formula;
			cell->formula = NULL;
		} else {
			cell->value = tableEvaluateFormula(table, target, cell->formula);
		}
	}

	debug(4, "### %p evaluated to %p\n", cell, cell->value);

	return cell->value;
}

Object * tableEvaluateCellRefs(Table * table, Cell * target, Cell * cell, List * refs) {
	Object * result = tableEvaluateCell(table, target, cell);
	listRemove(refs, (Object *) cell);
	return result;
}

int tableEvaluateRange(Table * table, Cell * target, Range * rng, List * dest) {
	debug(3, "%f(target = %p, rng = %p, dest = %p)\n", __FUNCTION__, target, rng, dest);
	for (int r = rng->start->row, re = rng->end->row; r <= re; r++) {
		for (int c = rng->start->col, ce = rng->end->col; c <= ce; c++) {
			Cell * ref = tableGet(table, c, r);
			if (ref) {
				if (ref == target) {
					debug(1, "loop detected for %p\n", target);
					if (!listAppend(dest, (Object *) table->eloop)) return 0;
					return 0;
				}
				Object * p = tableEvaluateCell(table, target, ref);
				if (p) {
					if (!listAppend(dest, p)) return 0;
				}
			}
		}
	}
	return 1;
}

Object * tableEvaluateCall(Table * table, Cell * target, Call * call) {
	debug(3, "%f(target = %p, call = %p)\n", __FUNCTION__, target, call);
	List * params = newList(table->c); if (!params) goto fail;
	List * args = call->args;

	// collect
	ListNode * node = args->first;
	while (node) {
		if (objectInstanceOf(node->value, &RangeVTable)) {
			Range * rng = (Range *) node->value;
			if (!tableEvaluateRange(table, target, rng, params)) goto fail;
		} else {
			// todo: NULL vs. empty!
			Object * p = tableEvaluateFormula(table, target, node->value);
			if (p) {
				if (!listAppend(params, p)) goto fail;
			}
		}
		node = node->next;
	}

	Object * res = tableDispatchCall(table, call->func->value, params);

	return res;

fail:
	return (Object *) table->ecall;
}

Object * tableRangeToList(Table* table, Cell * target, Range * rng) {
	debug(3, "%f(target = %p, rng = %p)\n", __FUNCTION__, target, rng);
	List * list = newList(table->c);
	if (!list) goto fail;
	if (!tableEvaluateRange(table, target, rng, list)) goto fail;

	return (Object *) list;
fail:
	return NULL;
}

Object * tableEvaluateFormula(Table * table, Cell * target, Object * obj) {
	debug(3, "%f(target = %p, obj = %p)\n", __FUNCTION__, target, obj);
	if (objectInstanceOf(obj, &CoordVTable)) {
		Coord * coord = (Coord *) obj;
		Cell * ref = tableGet(table, coord->col, coord->row);
		if (!ref) goto fail;
		if (ref == target) {
			debug(1, "loop detected for %p\n", target);
			return (Object *) table->eloop;
		}
		return tableEvaluateCell(table, target, ref);
	} else if (objectInstanceOf(obj, &RangeVTable)) {
		Range * rng = (Range *) obj;
		return tableRangeToList(table, target, rng);
	} else if (objectInstanceOf(obj, &CallVTable)) {
		Call * call = (Call *) obj;
		return tableEvaluateCall(table, target, call);
	} else if (objectInstanceOf(obj, &DoubleVTable)) {
		return obj;
	} else if (objectInstanceOf(obj, &StringVTable)) {
		return obj;
	}
fail:
	return (Object *) table->eformula;
}

ObjectVTable TableVTable = {
	.name = "Table",

	.parent = NULL,
	.toString = NULL,
	.free = tableFree,
	.propagateMark = tablePropagateMark,
};

Table * newTable(Collector * c, int cols, int rows) {
	if (cols < 0 || rows < 0 || cols > 26) return NULL;

	int ncells = cols * rows;
	Table * result = malloc(sizeof(Table));
	if (!result) return NULL;
	((Object *) result)->vt = &TableVTable;
	result->cols = cols;
	result->rows = rows;
	result->cells = malloc(sizeof(Cell *) * ncells);
	int i = 0;
	for (i = 0; i < ncells; i++) {
		Cell * cell = newCell(c, i % cols, i / cols);
		if (!cell) goto fail;
		result->cells[i] = cell;
	}

	result->ecall = newError(c, "#CALL");
	if (!result->ecall) goto fail;
	result->eerror = newError(c, "#ERROR");
	if (!result->eerror) goto fail;
	result->eformula = newError(c, "#FORMULA");
	if (!result->eformula) goto fail;
	result->elink = newError(c, "#LINK");
	if (!result->elink) goto fail;
	result->eloop = newError(c, "#LOOP");
	if (!result->eloop) goto fail;
	result->erefs = newError(c, "#REFS");
	if (!result->erefs) goto fail;
	result->eundefined = newError(c, "#UNDEFINED");
	if (!result->eundefined) goto fail;

	if (!collectorIntern(c, (Object *) result)) goto fail;
	result->c = c;
	return result;

fail:
	free(result->cells);
	free(result);
	return NULL;
}

void tableClear(Table * table) {
	debug(0, "%f()\n", __FUNCTION__);

	int ncells = table->cols * table->rows;
	for (int i = 0; i < ncells; i++) {
		Cell * cell = table->cells[i];
		cell->formula = NULL;
		cell->value = NULL;
		listRemoveAll(cell->depends);
		listRemoveAll(cell->propagate);
	}
}

// todo: clear invalid cells
int tableResize(Table * table, int cols, int rows) {
	debug(0, "%f(cols = %i, rows = %i)\n", __FUNCTION__, cols, rows);

	int oncells = table->cols * table->rows;
	int ncells = cols * rows;
	if (ncells == oncells) return 1;

	Cell ** cells = malloc(sizeof(Cell *) * ncells);
	if (!cells) return 0;

	for (int i = 0; i < ncells; i++) {
		int col = i % cols;
		int row = i / cols;

		Cell * cell = NULL;
		if (col >= table->cols || row >= table->rows) {
			cell = newCell(table->c, i % cols, i / cols);
			if (!cell) goto fail;
		} else {
			cell = table->cells[row * table->cols + col];
		}
		cells[i] = cell;
	}

	free(table->cells);
	table->cells = cells;
	table->cols = cols;
	table->rows = rows;

	return 1;

fail:
	free(cells);
	return 1;
}

static int tablePrepareRefs(Table * table, Cell * cell, List * refs) {
	debug(2, "%f(cell = %p)\n", __FUNCTION__, cell);

	ListNode * node = cell->propagate->first;
	while (node) {
		Cell * prop = (Cell *) node->value;
		if (prop->formula && !listContains(refs, (Object *) prop)) {
			prop->value = NULL;
			if (!listAppend(refs, (Object *) prop)) return 0;
			tablePrepareRefs(table, prop, refs);
		}
		node = node->next;
	}
	return 1;
}

static int tableCellLink(Table * table, Cell * cell) {
	(void) table;
	ListNode * node = cell->depends->first;
	while (node) {
		Cell * ref = (Cell *) node->value;
		if (!listContains(ref->propagate, (Object *) cell)) {
			if (!listAppend(ref->propagate, (Object *) cell)) return 0;
		}
		node = node->next;
	}
	return 1;
}

static int tableCellUnlink(Table * table, Cell * cell) {
	(void) table;
	while (!listIsEmpty(cell->depends)) {
		Cell * ref = (Cell *) listPop(cell->depends);
		listRemove(ref->propagate, (Object *) cell);
	}
	return 1;
}

void tablePropagateRefs(Table * table, List * refs) {
	debug(2, "%f(refs = %p)\n", __FUNCTION__, refs);

	while (!listIsEmpty(refs)) {
		Cell * cell = (Cell *) listPopN(refs, 0);
		tableEvaluateCellRefs(table, cell, cell, refs);
	}
}

void tableClearCell(Table * table, Cell * cell) {
	debug(0, "%f(cell = %p)\n", __FUNCTION__, cell);

	cell->formula = NULL;
	cell->value = NULL;

	tableCellUnlink(table, cell);

	List * refs = newList(table->c);
	if (!refs) return;
	if (!tablePrepareRefs(table, cell, refs)) return;
	tablePropagateRefs(table, refs);
}

int tableSetCell(Table * table, Cell * cell, Buffer * value) {
	debug(0, "%f(cell = %p, '%s')\n", __FUNCTION__, cell, value->data);

	if (value->usage) {
		cell->formula = newFormula(table->c, value);
		if (cell->formula) {
			if (objectInstanceOf(cell->formula, &PatternVTable)) {
				Cell * ref = patternGet((Pattern *) cell->formula, table, cell);
				if (!ref) {
					debug(1, "invalid reference %p\n", cell->formula);
					return 0;
				}
				cell->formula = (Object *) newCoord(table->c, ref->coord.col, ref->coord.row);
				if (!cell->formula) {
					debug(1, "could not create reference for %p\n", ref);
					return 0;
				}
				debug(1, "created reference %p for %p\n", cell->formula, ref);
			}

			tableCellUnlink(table, cell);
			if (!tableFormulaReferences(table, cell->formula, cell->depends)) return 0;
			if (!tableCellLink(table, cell)) return 0;
			cell->value = NULL;
			tableEvaluateCell(table, cell, cell);

			List * refs = newList(table->c);
			if (!refs) return 0;
			if (!tablePrepareRefs(table, cell, refs)) return 0;
			tablePropagateRefs(table, refs);
		} else {
			debug(1, "could not parse formula '%s'\n", value->data);
			return 0;
		}
	} else {
		tableClearCell(table, cell);
	}
	return 1;
}

int tableImport(Table * table, const char * file) {
	debug(0, "%f(file = '%s')\n", __FUNCTION__, file);

	int status = 0;
	FILE * fp = NULL;
	Buffer * buffer = NULL;
	Buffer * value = NULL;

	buffer = newBuffer(1024);
	if (!buffer) goto cleanup;
	value = newBuffer(1024);
	if (!value) goto cleanup;

	fp = fopen(file, "r");
	if (!fp) {
		debug(1, "could not open file '%s' for reading!\n", file);
		goto cleanup;
	}

	Tokenizer ts;
	int rows = 0, row;
	int cols = 0, col;
	while (!feof(fp)) {
		bufferReset(buffer);
		if (!bufferReadLine(buffer, fp)) break;
		if (bufferIsEmpty(buffer)) continue;
		if (buffer->data[0] == '#') continue;
		rows++;
		if (rows == 1) {
			cols = 1;
			tokenizerInit(&ts, buffer);
			TokenType t;
			while ((t = tokenizerNext(&ts))) {
				if (t == TOK_SEMICOLON) cols++;
			}
		}
	}

	debug(2, "resizing to (%i,%i)\n", cols, rows);

	tableClear(table);
	if (!tableResize(table, cols, rows)) {
		debug(1, "could not resize table!\n");
		goto cleanup;
	}

	clearerr(fp);
	fseek(fp, 0, SEEK_SET);

	int trunc = 0;

	row = 0;
	while (!feof(fp)) {
		bufferReset(buffer);
		if (!bufferReadLine(buffer, fp)) break;
		if (bufferIsEmpty(buffer)) continue;
		if (buffer->data[0] == '#') continue;
		tokenizerInit(&ts, buffer);
		TokenType t;
		char * s = buffer->data;
		col = 0;
		while ((t = tokenizerNext(&ts))) {
			if (t == TOK_SEMICOLON) {
				bufferReset(value);
				if (!bufferWrite(value, (ts.bp - 1 - s), s)) goto cleanup;
				if (col < cols && row < rows) {
					if (!tableSetCell(table, tableGet(table, col, row), value)) goto cleanup;
				} else {
					trunc = 1;
				}
				s = ts.bp;
				col++;
			}
		}
		bufferReset(value);
		if (!bufferWrite(value, (ts.be - s), s)) goto cleanup;
		if (col < cols && row < rows) {
			if (!tableSetCell(table, tableGet(table, col, row), value)) goto cleanup;
		} else {
			trunc = 1;
		}
		row++;
	}

	if (trunc) {
		debug(1, "table parsed with errors! expect truncated rows.\n");
	} else {
		status = 1;
	}

cleanup:
	if (fp) fclose(fp);
	if (buffer) bufferFree(buffer);
	if (value) bufferFree(value);

	return status;
}

static int tableExportCell(Cell * cell, Buffer * buff) {
	char * s = NULL;
	char * m = NULL;
	if (cell->formula) {
		if (!bufferWrite(buff, 1, "=")) goto fail;
		s = objectToString((Object *) cell->formula);
	} else if (cell->value) {
		if (objectInstanceOf(cell->value, &StringVTable)) {
			m = "\"";
		}
		s = objectToString((Object *) cell->value);
	} else {
		return 1;
	}

	if (s) {
		if (m) {
			if (!bufferWrite(buff, strlen(m), m)) goto fail;
		}
		if (!bufferWrite(buff, strlen(s), s)) goto fail;
		if (m) {
			if (!bufferWrite(buff, strlen(m), m)) goto fail;
		}
		free(s);
		return 1;
	}

fail:
	if (s) free(s);
	return 0;
}

/*
 exports CSV, so that cells are separated by ';' and lines by '\n'
 ┏━┳━━━┳━━━┳━━━┓
 ┃ ┃  A┃  B┃  C┃
 ┣━╋━━━╋━━━╋━━━┫
 ┃1┃  1┃  2┃  3┃
 ┃2┃  A┃  B┃  C┃
 ┗━┻━━━┻━━━┻━━━┛

 becomes

 1;2;=sum(A1,B1)
 "A";"B";"C"

*/

int tableExport(Table * table, const char * file) {
	debug(0, "%f(file = '%s')\n", __FUNCTION__, file);

	int status = 0;
	Buffer * buffer = newBuffer(1024);
	if (!buffer) return 1;

	FILE * fp = fopen(file, "w");
	if (!fp) {
		debug(1, "could not open file '%s' for writing!\n", file);
		goto cleanup;
	}

	int cols = table->cols;
	int rows = table->rows;

	for (int r = 0; r < rows; r++) {
		bufferReset(buffer);
		for (int c = 0; c < cols; c++) {
			if (c > 0) {
				if (!bufferWrite(buffer, 1, ";")) goto cleanup;
			}
			Cell * cell = tableGet(table, c, r);
			if (!tableExportCell(cell, buffer)) goto cleanup;
		}
		if (!bufferWrite(buffer, 1, "\n")) goto cleanup;
		if (fputs(buffer->data, fp) < 0) {
			debug(1, "error while writing '%s'\n", file);
			goto cleanup;
		}
	}

	status = 1;

cleanup:
	if (fp) fclose(fp);
	if (buffer) bufferFree(buffer);

	return status;
}
