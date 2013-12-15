#ifndef BUFFER_INC_C
#define BUFFER_INC_C

#include "Attribute.h"

#include <stdlib.h>
#include <stdio.h>

#define ERROR 0
#define SUCCESS 1

typedef struct {
	size_t size;
	size_t usage;
	char * data;
} Buffer;

typedef enum {
	BUFFER_SPLIT_EXACT = 0, // "a<sep><sep>b" -> "a" "" "b"
	BUFFER_SPLIT_MERGE = 1, // "a<sep><sep>b" -> "a" "b"
	BUFFER_SPLIT_TAKE  = 2, // take the tail and leave :-D
} BufferSplit;


APIUSE Buffer * newBuffer(size_t size);
void bufferReset(Buffer * b);
int bufferIsEmpty(Buffer * b);
APIUSE int bufferWrite(Buffer * b, size_t length, const char data[length]);
void bufferFree(Buffer * b);
int bufferSplitN(const Buffer * b, char sep, unsigned type, int n, /* Buffer** */...);
int bufferReadLine(Buffer * buffer, FILE * fp);

#endif /* BUFFER_INC_C */
