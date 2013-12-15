#include "Buffer.h"

#include <stdarg.h>

Buffer * newBuffer(size_t size) {
	Buffer * b = NULL;

	b = malloc(sizeof(Buffer));
	if (!b) goto fail;

	b->data = malloc(size + 1);
	if (!b->data) goto fail;

	b->usage = 0;
	b->size = size;
	b->data[0] = 0;

	return b;

fail:
	if (b) free(b);
	return NULL;
}

void bufferReset(Buffer * b) {
	b->usage = 0;
}

int bufferIsEmpty(Buffer * b) {
	return b->usage == 0;
}

int bufferWrite(Buffer * b, size_t length, const char data[length]) {
	size_t need = b->usage + length + 1;
	if (need > b->size) {
		char * tmp = realloc(b->data, need);
		if (!tmp) return ERROR;
		b->data = tmp;
		b->size = need;
	}

	const char * s = data, * e = data + length;
	for (char * d = b->data + b->usage; s < e; ) {
		*(d++) = *(s++);
	}
	b->usage += length;
	b->data[b->usage]= 0;
	return SUCCESS;
}

void bufferFree(Buffer * b) {
	free(b->data);
	free(b);
}

int bufferSplitN(const Buffer * b, char sep, unsigned type, int n, /* Buffer** */...) {
	enum {
		Z_FIELD,
		Z_SEP,
	} z = Z_FIELD;

	if (n < 1) return ERROR;
	if (!b->usage) return ERROR;

	struct {
		Buffer * b;
		const char * start;
		const char * end;
	} state[n];

	for (int i = 0; i < n; i++) {
		state[i].b = NULL;
	}

	int f = 0;
	const char * s = b->data;
	const char * e = s + b->usage;
	int merge = (type & BUFFER_SPLIT_MERGE);
	int take = (type & BUFFER_SPLIT_TAKE);
	const char * p = s;

	while(f < n && p < e) {
		if (take && z == Z_FIELD && !(f + 1 < n)) break;

		char c = *p;

		switch (z) {
			case Z_FIELD:
				if (c == sep) {
					goto field_end;
				} else {
					goto next;
				}
			case Z_SEP:
				if (c == sep) {
					if (!merge) {
						goto field_sep;
					}
					goto next;
				} else {
					goto field_start;
				}
		}
field_sep: // epsilon transistion!
		z = Z_FIELD;
		f++;
		s = p;
		continue;
field_start:
		z = Z_FIELD;
		f++;
		s = p;
		p++;
		continue;
field_end:
		z = Z_SEP;
		state[f].start = s;
		state[f].end = p;
		p++;
		continue;
next:
		p++;
		continue;
	}

	// make last epsilon transistion
	if (p == e && z == Z_SEP && (f + 1 < n)) {
		z = Z_FIELD;
		f++;
		s = p;
	}

	if (z != Z_FIELD || f + 1 != n) goto fail;

	// finish transistion
	state[f].start = s;
	if (!take) {
		state[f].end = p;
	} else  {
		state[f].end = e;
	}

	va_list ap;
	va_start(ap, n);
	for (int i = 0; i < n; i++) {
		size_t length = state[i].end - state[i].start;
		if (!(state[i].b = newBuffer(length))) goto fail_va;
		if (!bufferWrite(state[i].b, length, state[i].start)) goto fail_va;
		Buffer ** pp = va_arg(ap, Buffer **);
		*pp = state[i].b;
	}
	va_end(ap);

	return SUCCESS;

fail_va:
	va_end(ap);
fail:
	for (int i = 0; i < n; i++) {
		Buffer * buffer = state[i].b;
		if (buffer) bufferFree(buffer);
	}

	return ERROR;
}

int bufferReadLine(Buffer * buffer, FILE * fp) {
	do {
		int c = getc(fp);
		switch (c) {
			case EOF:
				return 1;
			case '\n':
				return 1;
			default: {
				char b = c;
				if (!bufferWrite(buffer, 1, &b)) return 0;
			}
		}
	} while (1);
}
