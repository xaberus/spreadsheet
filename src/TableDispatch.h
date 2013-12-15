#ifndef TABLEDISPATCH_H
#define TABLEDISPATCH_H

#include "Object.h"
#include "List.h"
#include "Table.h"

APIUSE Object * tableDispatchCall(Table * table, const char * func, List * params);

#endif /* TABLEDISPATCH_H */
