#include "Table.h"
#include "List.h"
#include "Double.h"
#include "String.h"
#include "Call.h"
#include "Range.h"

#include <string.h>
#include <stdio.h>

typedef struct {
	int first;
	int error;
	double value;
} ArihmeticContext;

static void dispatchSumCallback(Object * obj, void * ctx) {
	ArihmeticContext * ac = ctx;
	Double * d = (Double *) obj;
	ac->value += d->value;
}

static void dispatchSubCallback(Object * obj, void * ctx) {
	ArihmeticContext * ac = ctx;
	Double * d = (Double *) obj;
	if (ac->first) {
		ac->value = d->value;
		ac->first = 0;
		ac->error = 0;
	} else if (!ac->error) {
		ac->value -= d->value;
	}
}

static void dispatchMulCallback(Object * obj, void * ctx) {
	ArihmeticContext * ac = ctx;
	Double * d = (Double *) obj;
	ac->value *= d->value;
}

static void dispatchDivCallback(Object * obj, void * ctx) {
	ArihmeticContext * ac = ctx;
	Double * d = (Double *) obj;
	if (ac->first) {
		ac->value = d->value;
		ac->first = 0;
		ac->error = 0;
	} else if (!ac->error) {
		if (d->value == 0) {
			ac->error = 1;
			return;
		}
		ac->value /= d->value;
	}
}

static void dispatchMinCallback(Object * obj, void * ctx) {
	ArihmeticContext * ac = ctx;
	Double * d = (Double *) obj;
	if (ac->first) {
		ac->value = d->value;
		ac->first = 0;
		ac->error = 0;
	} else if (!ac->error) {
		if (ac->value > d->value) ac->value = d->value;
	}
}

static void dispatchMaxCallback(Object * obj, void * ctx) {
	ArihmeticContext * ac = ctx;
	Double * d = (Double *) obj;
	if (ac->first) {
		ac->value = d->value;
		ac->first = 0;
		ac->error = 0;
	} else if (!ac->error) {
		if (ac->value < d->value) ac->value = d->value;
	}
}

static const struct {
	const char * cmd;
	ListElementFunction fn;
	double init;
	int error;
} arithmeticTable[] = {
	{"sum", dispatchSumCallback, 0.0, 0},
	{"sub", dispatchSubCallback, 0.0, 1},
	{"mul", dispatchMulCallback, 1.0, 0},
	{"div", dispatchDivCallback, 1.0, 1},
	{"min", dispatchMinCallback, 0, 1},
	{"max", dispatchMaxCallback, 0, 1},
};

typedef Object * (*Dispatcher)(Collector * c, List * list);

static Object * dispatchList(Collector * c, List * list) {
	(void) c;
	return (Object *) list;
}

static int dispatchStringsCallback(Object * obj, void * ctx) {
	(void) ctx;
	if (objectInstanceOf(obj, &StringVTable)) {
		return 1;
	}
	return 0;
}

static Object * dispatchStrings(Collector * c, List * list) {
	List * strings = listFilter(c, list, dispatchStringsCallback, NULL);
	if (!strings) return NULL;
	String * res = newStringOwning(c, objectToString((Object *) strings));
	return (Object *) res;
}

static const struct {
	const char * cmd;
	Dispatcher fn;
} callTable[] = {
	{"list", dispatchList},
	{"strings", dispatchStrings},
};

static int isDouble(Object * obj, void * ctx) {
	(void) ctx;
	return (objectInstanceOf(obj, &DoubleVTable));
}

Object * tableDispatchCall(Table * table, const char * func, List * params) {
#if 0
	/**/{
	/**/	char * s = objectToString((Object *) params);
	/**/	if (s) {
	/**/		printf("params: %s\n", s);
	/**/		free(s);
	/**/	}
	/**/}
#endif

	// check for errors

	ListNode * node = params->first;
	while (node) {
		if (objectInstanceOf(node->value, &ErrorVTable)) {
			return node->value;
		}
		node = node->next;
	}

	for (unsigned i = 0; i < sizeof(callTable)/sizeof(callTable[0]); i++) {
		if (strcmp(callTable[i].cmd, func) == 0) {
			return callTable[i].fn(table->c, params);
		}
	}
	for (unsigned i = 0; i < sizeof(arithmeticTable)/sizeof(arithmeticTable[0]); i++) {
		if (strcmp(arithmeticTable[i].cmd, func) == 0) {
			List * numbers = listFlatFilter(table->c, params, isDouble, NULL);
			if (!numbers) return (Object *) table->eerror;
			ArihmeticContext ctx = {
				.first = 1,
				.error = arithmeticTable[i].error,
				.value = arithmeticTable[i].init,
			};
			if (!listIsEmpty(numbers)) {
				listFlatForEach(numbers, arithmeticTable[i].fn, &ctx);
			}
			if (!ctx.error) {
				return (Object *) newDouble(table->c, ctx.value);
			} else {
				return (Object *) table->eerror;
			}
		}
	}
	/**/ printf("undefined function '%s'\n", func);
	return (Object *) table->eundefined;
}
