#ifndef DOUBLE_H
#define DOUBLE_H

#include "Object.h"

typedef struct {
	Object object;
	double value;
} Double;

extern ObjectVTable DoubleVTable;

APIUSE Double * newDouble(Collector *c, double d);

#endif /* DOUBLE_H */
