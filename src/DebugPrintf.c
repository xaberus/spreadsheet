#include "DebugPrintf.h"
#include "Buffer.h"
#include "Object.h"

#include <stdarg.h>
#include <string.h>

void debugPrintf(FILE * fp, const char * fmt, ...) {
	Buffer * buffer = newBuffer(1024);
	if (!buffer) return;

	enum {
		STATE_START,
		STATE_PERC,
	} state = STATE_START;

	va_list ap;

	va_start(ap, fmt);
	const char * p = fmt;
	char c;

	const char * col_red = "\x1b[1;31m";
	const char * col_green = "\x1b[1;32m";
	const char * col_yellow = "\x1b[1;33m";
	const char * col_none = "\x1b[0m";

	char * s = NULL;

	while (p && (c = *p)) {
		if (s) free(s); s = NULL;

		switch (state) {
			case STATE_START:
				if (c == '%') goto perc;
				if (!bufferWrite(buffer, 1, p)) goto out;
				goto start;
			case STATE_PERC:
				switch (c) {
					case 'f': {
						const char * p = va_arg(ap, const char *);
						if (!bufferWrite(buffer, strlen(col_yellow), col_yellow)) goto out;
						if (!bufferWrite(buffer, strlen(p), p)) goto out;
						if (!bufferWrite(buffer, strlen(col_none), col_none)) goto out;
						goto start;
					}
					case 's': {
						const char * p = va_arg(ap, const char *);
						if (!bufferWrite(buffer, strlen(p), p)) goto out;
						goto start;
					};
					case 'z': {
						size_t p = va_arg(ap, size_t);
						char tmp[sizeof(p)*8*13];
						snprintf(tmp, sizeof(tmp), "%zu", p);
						if (!bufferWrite(buffer, strlen(tmp), tmp)) goto out;
						goto start;
					};
					case 'i': {
						int p = va_arg(ap, int);
						char tmp[sizeof(p)*8*13];
						snprintf(tmp, sizeof(tmp), "%d", p);
						if (!bufferWrite(buffer, strlen(tmp), tmp)) goto out;
						goto start;
					};
					case 'I': {
						unsigned int p = va_arg(ap, unsigned int);
						char tmp[sizeof(p)*8*13];
						snprintf(tmp, sizeof(tmp), "%u", p);
						if (!bufferWrite(buffer, strlen(tmp), tmp)) goto out;
						goto start;
					};
					case 'l': {
						long p = va_arg(ap, long);
						char tmp[sizeof(p)*8*13];
						snprintf(tmp, sizeof(tmp), "%ld", p);
						if (!bufferWrite(buffer, strlen(tmp), tmp)) goto out;
						goto start;
					};
					case 'L': {
						unsigned long p = va_arg(ap, unsigned long);
						char tmp[sizeof(p)*8*13];
						snprintf(tmp, sizeof(tmp), "%lu", p);
						if (!bufferWrite(buffer, strlen(tmp), tmp)) goto out;
						goto start;
					};
					case 'g': {
						double p = va_arg(ap, double);
						char tmp[sizeof(p)*8*13];
						snprintf(tmp, sizeof(tmp), "%g", p);
						if (!bufferWrite(buffer, strlen(tmp), tmp)) goto out;
						goto start;
					};
					case 'o': {
						Object * o = va_arg(ap, Object *);
						if (o) {
							const char * t = objectTypeName(o);
							s = objectToString(o);
							if (!s) goto out;
							if (!bufferWrite(buffer, 1, "(")) goto out;
							if (!bufferWrite(buffer, strlen(col_green), col_red)) goto out;
							if (!bufferWrite(buffer, strlen(t), t)) goto out;
							if (!bufferWrite(buffer, strlen(col_none), col_none)) goto out;
							if (!bufferWrite(buffer, 2, ") ")) goto out;
							if (!bufferWrite(buffer, strlen(col_green), col_green)) goto out;
							if (!bufferWrite(buffer, strlen(s), s)) goto out;
							if (!bufferWrite(buffer, strlen(col_none), col_none)) goto out;
						} else {
							if (!bufferWrite(buffer, 3, "nil")) goto out;
						}
						goto start;
					}
					case 'p': {
						Object * o = va_arg(ap, Object *);
						if (o) {
							if (!bufferWrite(buffer, strlen(col_green), col_green)) goto out;
							s = objectToString(o);
							if (!s) goto out;
							if (!bufferWrite(buffer, strlen(s), s)) goto out;
							if (!bufferWrite(buffer, strlen(col_none), col_none)) goto out;
						} else {
							if (!bufferWrite(buffer, 3, "nil")) goto out;
						}
						goto start;
					}
					default:
						goto out;
				}
		}
start:
		state = STATE_START;
		p++;
		continue;
perc:
		state = STATE_PERC;
		p++;
		continue;
	}

	fputs(buffer->data, fp);
	fflush(fp);

out:
	if (s) free(s);

	va_end(ap);
	bufferFree(buffer);
}
