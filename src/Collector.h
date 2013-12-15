#ifndef COLLECTOR_H
#define COLLECTOR_H

#include "Attribute.h"

#include <stdlib.h>

/* poor man's garbage collection */

typedef struct Collector Collector;
typedef struct Object Object;

Collector * newCollector();
void collectorFree(Collector * c);

void collectorRun(Collector * c);
void collectorMark(Collector * c, Object * o);

APIUSE int collectorRoot(Collector * c, Object * o);
APIUSE int collectorIntern(Collector * c, Object * o);

#endif /* COLLECTOR_H */
