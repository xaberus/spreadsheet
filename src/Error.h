#ifndef ERROR_H
#define ERROR_H

#include "Object.h"

typedef struct {
	Object object;
	char * value;
} Error;

extern ObjectVTable ErrorVTable;

APIUSE Error * newError(Collector * c, const char * string);

#endif /* ERROR_H */
