
#include "Double.h"
#include "List.h"
#include "Coord.h"
#include "Range.h"
#include "Cell.h"
#include "String.h"
#include "Call.h"
#include "Table.h"

#include "Buffer.h"
#include "Formula.h"
#include "Collector.h"


#include <assert.h>
#include <string.h>
#include <stdio.h>

const int testCollector = 1;

const int testBuffer = 1;
const int testList = 1;
const int testCoord = 1;
const int testRange = 1;
const int testCell = 1;
const int testCall = 1;
const int testTable = 1;

const int testFormula = 1;

int main() {
#define prepare(str) \
	do {\
		bufferReset(b); \
		assert(bufferWrite(b, strlen(str), str)); \
		printf("### in '%.*s'\n", (int) b->usage, b->data); \
	} while(0)
#define check(target, str) \
	do {\
		/*printf(">>> out '%.*s'\n", (int) target->usage, target->data);*/ \
		assert(strcmp(str, target->data) == 0); \
	} while(0)

	if (testBuffer) {
		Buffer * b = newBuffer(1024);
		Buffer * x, * y, * z;

		prepare("a123  b345   c789");
		assert(bufferSplitN(b, ' ', BUFFER_SPLIT_MERGE, 3, &x, &y, &z));
		check(x, "a123");
		bufferFree(x);
		check(y, "b345");
		bufferFree(y);
		check(z, "c789");
		bufferFree(z);

		prepare("");
		assert(!bufferSplitN(b, ' ', BUFFER_SPLIT_MERGE, 3, &x, &y, &z));

		prepare("a b c ");
		assert(!bufferSplitN(b, ' ', BUFFER_SPLIT_MERGE, 3, &x, &y, &z));
		assert(bufferSplitN(b, ' ', BUFFER_SPLIT_MERGE|BUFFER_SPLIT_TAKE, 3, &x, &y, &z));
		check(x, "a");
		bufferFree(x);
		check(y, "b");
		bufferFree(y);
		check(z, "c ");
		bufferFree(z);

		prepare("a,b,c");
		assert(bufferSplitN(b, ',', BUFFER_SPLIT_MERGE, 3, &x, &y, &z));
		check(x, "a");
		bufferFree(x);
		check(y, "b");
		bufferFree(y);
		check(z, "c");
		bufferFree(z);

		prepare("a,b,c");
		assert(bufferSplitN(b, ',', BUFFER_SPLIT_EXACT, 3, &x, &y, &z));
		check(x, "a");
		bufferFree(x);
		check(y, "b");
		bufferFree(y);
		check(z, "c");
		bufferFree(z);

		prepare("a,b,c,d");
		assert(bufferSplitN(b, ',', BUFFER_SPLIT_EXACT|BUFFER_SPLIT_TAKE, 3, &x, &y, &z));
		check(x, "a");
		bufferFree(x);
		check(y, "b");
		bufferFree(y);
		check(z, "c,d");
		bufferFree(z);

		prepare(",,");
		assert(bufferSplitN(b, ',', BUFFER_SPLIT_EXACT, 3, &x, &y, &z));
		check(x, "");
		bufferFree(x);
		check(y, "");
		bufferFree(y);
		check(z, "");
		bufferFree(z);

		prepare(",,,,,,,,,");
		assert(!bufferSplitN(b, ',', BUFFER_SPLIT_EXACT, 3, &x, &y, &z));
		assert(bufferSplitN(b, ',', BUFFER_SPLIT_EXACT|BUFFER_SPLIT_TAKE, 3, &x, &y, &z));
		check(x, "");
		bufferFree(x);
		check(y, "");
		bufferFree(y);
		check(z, ",,,,,,,");
		bufferFree(z);

		prepare("a");
		assert(bufferSplitN(b, ',', BUFFER_SPLIT_EXACT, 1, &x));
		check(x, "a");
		bufferFree(x);

		bufferFree(b);
	}

	if (testBuffer) {
		const char * msg = "a123,b345,c789";
		Buffer * buffer = newBuffer(strlen(msg));
		assert(bufferWrite(buffer, strlen(msg), msg));

		Buffer * a, * b, * c, * d;

		printf("### in '%.*s'\n", (int) buffer->usage, buffer->data);

		assert(bufferSplitN(buffer, ',', BUFFER_SPLIT_EXACT, 3, &a, &b, &c) == SUCCESS);
		bufferFree(a);
		bufferFree(b);
		bufferFree(c);

		assert(bufferSplitN(buffer, ',', BUFFER_SPLIT_EXACT, 2, &a, &b, &c) == ERROR);
		assert(bufferSplitN(buffer, ',', BUFFER_SPLIT_EXACT, 2, &a, &b, &c, &d) == ERROR);

		bufferFree(buffer);
	}

	if (testBuffer) {
		const char * msg = "a123,,b456,c7890";
		Buffer * buffer = newBuffer(strlen(msg));
		assert(bufferWrite(buffer, strlen(msg), msg));

		Buffer * a, * b, * c, * d;

		printf("### in '%.*s'\n", (int) buffer->usage, buffer->data);

		assert(bufferSplitN(buffer, ',', BUFFER_SPLIT_EXACT, 4, &a, &b, &c, &d) == SUCCESS);
		bufferFree(a);
		bufferFree(b);
		bufferFree(c);
		bufferFree(d);

		assert(bufferSplitN(buffer, ',', BUFFER_SPLIT_MERGE, 3, &a, &b, &c) == SUCCESS);
		bufferFree(a);
		bufferFree(b);
		bufferFree(c);

		assert(bufferSplitN(buffer, ',', BUFFER_SPLIT_EXACT, 3, &a, &b, &c) == ERROR);

		bufferFree(buffer);
	}


	if (testCollector && testCell) {
		Collector * c = newCollector();
		Cell * t[100];
		for (unsigned i = 0; i < sizeof(t)/sizeof(t[0]); i++) {
			assert(t[i] = newCell(c, 0,0));
		}
		Cell * root;
		assert(root = newCell(c, 0,0));
		root->formula = (Object *) newDouble(c, 1.1);
		root->value = (Object *) newDouble(c, 1.1);
		for (unsigned i = 0; i < sizeof(t)/sizeof(t[0]); i++) {
			assert(listAppend(root->propagate, (Object *) t[i]));
		}
		assert(collectorRoot(c, (Object *) root));
		collectorRun(c);
		for (unsigned i = 0; i < sizeof(t)/sizeof(t[0]); i++) {
			assert(t[i]->propagate);
		}
		collectorFree(c);
	}


	if (testCollector && testList) {
		Collector * c = newCollector();
		List * l = newList(c);
		for (int i = 0; i < 100; i++) assert(listAppend(l, (Object *) newDouble(c, i)));
		assert(listLength(l) == 100);
		collectorFree(c);
	}

	if (testCollector && testList) {
		Collector * c = newCollector();
		List * l = newList(c);
		for (int i = 0; i < 100; i++) assert(listAppend(l, (Object *) newDouble(c, i)));
		for (int i = 0; i < 50; i++) {
			Double * d = (Double *) listPopN(l, i);
			assert(d && d->value == 2*i);
		}
		collectorFree(c);
	}

	if (testCollector && testList) {
		Collector * c = newCollector();
		List * l = newList(c);
		for (int i = 0; i < 100; i++) assert(listAppend(l, (Object *) newDouble(c, i)));
		for (int i = 0; i < 50; i++) {
			Double * d = (Double *) listPop(l);
			assert(d && d->value == 99 - i);
		}
		collectorFree(c);
	}

	if (testCollector && testList) {
		Collector * c = newCollector();
		List * l = newList(c);
		for (int i = 0; i < 100; i++) assert(listAppend(l, (Object *) newDouble(c, i)));
		for (int i = 0; i < 50; i++) {
			Double * d = (Double *) listGet(l, i);
			assert(d && d->value == 2*i);
			listRemove(l, (Object *) d);
		}
		collectorFree(c);
	}

	if (testCollector && testList) {
		Collector * c = newCollector();
		List * l = newList(c);
		for (int i = 0; i < 100; i++) assert(listAppend(l, (Object *) newDouble(c, i)));
		for (int i = 0; i < 50; i++) {
			Double * d = (Double *) listGet(l, -1);
			assert(d && d->value == 99 - i);
			listRemove(l, (Object *) d);
		}
		collectorFree(c);
	}

	if (testCollector && testList) {
		Collector * c = newCollector();
		List * l = newList(c);
		for (int i = 0; i < 100; i++) assert(listAppend(l, (Object *) newDouble(c, i)));
		for (int i = 0; i < 100; i++) assert(listContains(l, listGet(l, i)));
		collectorFree(c);
	}

	if (testCollector && testList) {
		Collector * c = newCollector();
		List * l = newList(c);
		for (int i = 0; i < 100; i++) assert(listAppend(l, (Object *) newDouble(c, i)));
		for (int i = 0; i < 50; i++) {
			Double * d = (Double *) listGet(l, -(i+1));
			assert(d && d->value == 99 - 2*i);
			listRemove(l, (Object *) d);
		}
		collectorFree(c);
	}

	if (testCollector && testCoord) {
		Collector * c = newCollector();
		Coord * coord = newCoord(c, 0, 0);
		char * s = objectToString((Object *) coord);
		assert(strcmp(s, "A1") == 0);
		free(s);
		collectorFree(c);
	}

	if (testCollector && testRange) {
		Collector * c = newCollector();
		Coord * coord1 = newCoord(c, 0, 0);
		assert(coord1);
		Coord * coord2 = newCoord(c, 10, 10);
		assert(coord2);
		Range * r = newRange(c, coord1, coord2);
		char * s = objectToString((Object *) r);
		assert(strcmp(s, "A1:K11") == 0);
		free(s);
		collectorFree(c);
	}

	if (testCollector && testCall) {
		Collector * c = newCollector();
		String * func = newString(c, "func");
		List * args = newList(c);
		assert(listAppend(args, (Object *) newDouble(c, 1)));
		assert(listAppend(args, (Object *) newDouble(c, 2)));
		assert(listAppend(args, (Object *) newDouble(c, 3)));
		Call * cell = newCall(c, func, args);
		char * s = objectToString((Object *) cell);
		assert(strcmp(s, "func(1, 2, 3)") == 0);
		free(s);
		collectorFree(c);
	}


	if (testTable) {
		Collector * c = newCollector();
		assert(c);
		Table * t = newTable(c, 26, 100);
		assert(t);
		collectorFree(c);
	}

	/*if (testCollector && testFormula) {
		Collector * c = newCollector();
		Buffer * b = newBuffer(100);
		prepare("=sum(A1)");
		Object * o = newFormula(c, b);
		assert(o);
		collectorFree(c);
		bufferFree(b);
	}*/

	if (testCollector && testFormula) {
		Collector * c = newCollector();
		Buffer * b = newBuffer(100);
		/*prepare("=B3:B5");
		Object * o = newFormula(c, b);
		assert(o);*/
		assert(newRange(c, newCoord(c, 0,0), newCoord(c, 1,1)));
		collectorFree(c);
		bufferFree(b);
	}

	if (testCollector && testFormula) {
		Collector * c = newCollector();
		Buffer * b = newBuffer(100);
		prepare("=sum(A1, A2, 3, B3:B5, sum(3, 5))");
		Object * o = newFormula(c, b);
		assert(o);
		collectorFree(c);
		bufferFree(b);
	}

	if (testCollector && testFormula) {
		Collector * c = newCollector();
		Buffer * b = newBuffer(100);
		prepare("=blah");
		Object * o = newFormula(c, b);
		assert(!o);
		collectorFree(c);
		bufferFree(b);
	}

#define testnumber(str, num, tok) \
	do {\
		prepare(str);\
		tokenizerInit(&tt, b); \
		assert(tokenizerNext(&tt) == tok); \
		assert(tt.c.number == num); \
	} while(0)
	if (testFormula) {
		Buffer * b = newBuffer(100);
		Tokenizer tt;

		testnumber("1", 1, TOK_NUMBER);
		testnumber("-1", -1, TOK_NUMBER);
		testnumber("-.1", -.1, TOK_NUMBER);
		testnumber("-.0", -0, TOK_NUMBER);
		testnumber("-0.001", -0.001, TOK_NUMBER);
		testnumber("1.1", 1.1, TOK_NUMBER);
		testnumber("123424.1", 123424.1, TOK_NUMBER);

		testnumber("1e3", 1000, TOK_NUMBER);
		testnumber("1.1e3", 1100, TOK_NUMBER);
		testnumber("1e-3", 0.001, TOK_NUMBER);
		testnumber("-1e-3", -0.001, TOK_NUMBER);

		testnumber("A1", 1, TOK_CELLID);
		testnumber("B123", 123, TOK_CELLID);

		bufferFree(b);
	}
}
