#ifndef STRING_H
#define STRING_H

#include "Object.h"

typedef struct {
	Object object;
	char * value;
} String;

extern ObjectVTable StringVTable;

APIUSE String * newString(Collector * c, const char * string);
APIUSE String * newStringOwning(Collector * c, char * string);
APIUSE String * newStringN(Collector * c, int len, const char string[len]);

#endif /* STRING_H */
