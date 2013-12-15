#ifndef CALL_H
#define CALL_H

#include "Object.h"
#include "String.h"
#include "List.h"

typedef struct {
	Object object;
	String * func;
	List * args;
} Call;

extern ObjectVTable CallVTable;

APIUSE Call * newCall(Collector * c, String * func, List * args);

#endif /* CALL_H */
