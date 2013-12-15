#include "Strtab.h"
#include "Buffer.h"

#include <string.h>

void strtabPrintUtf8(FILE * fp, int cols, int rows, char ** strtab) {
	int colw[cols];
	Buffer * line = NULL;
	int linelen = 0;
	int colmax = 0;

	const int boxw = 3; // ┏,━,┳,┃,╋,┫,┣,┻,┻

	for (int i = 0; i < cols; i++) colw[i] = 3; // "  A"

	for (int c = 0; c < cols; c++) {
		for (int r = 0; r < rows; r++) {
			char * s = strtab[r * cols + c];
			if (!s) continue;
			int l = strlen(s);
			if (l > colw[c]) colw[c] = l;
		}
	}

	for (int i = 0; i < cols; i++) {
		line += colw[i]*boxw + boxw; // │<col>
		if (colw[i] > colmax) colmax = colw[i];
	}

	colmax += boxw + 1; // '\0'

	line = newBuffer(linelen);
	if (!line) return;

	char buffer[colmax > 64 ? colmax : 64];
	int numcolw = snprintf(buffer, sizeof(buffer), "%d", rows);

	// pre header
	bufferReset(line);
	for (int c = -1; c < cols; c++) {
		int len;
		if (c == -1) {
			if (!bufferWrite(line, boxw, "┏")) goto cleanup;
			len = numcolw;
		} else {
			if (!bufferWrite(line, boxw, "┳")) goto cleanup;
			len = colw[c];
		}
		for (int i = 0; i < len; i++) {
			if (!bufferWrite(line, boxw, "━")) goto cleanup;
		}
	}
	if (!bufferWrite(line, boxw, "┓")) goto cleanup;
	puts(line->data);

	// header
	bufferReset(line);
	snprintf(buffer, sizeof(buffer), "┃%*s", numcolw, "");
	if (!bufferWrite(line, numcolw + boxw, buffer)) goto cleanup;
	for (int c = 0; c < cols; c++) {
		snprintf(buffer, sizeof(buffer), "┃%*c", colw[c], c + 'A');
		if (!bufferWrite(line, colw[c] + boxw, buffer)) goto cleanup;
	}
	if (!bufferWrite(line, boxw, "┃")) goto cleanup;
	puts(line->data);

	// post header
	bufferReset(line);
	for (int c = -1; c < cols; c++) {
		int len;
		if (c == -1) {
			if (!bufferWrite(line, boxw, "┣")) goto cleanup;
			len = numcolw;
		} else {
			if (!bufferWrite(line, boxw, "╋")) goto cleanup;
			len = colw[c];
		}
		for (int i = 0; i < len; i++) {
			if (!bufferWrite(line, boxw, "━")) goto cleanup;
		}
	}
	if (!bufferWrite(line, boxw, "┫")) goto cleanup;
	puts(line->data);

	// data
	for (int r = 0; r < rows; r++) {
		bufferReset(line);
		snprintf(buffer, sizeof(buffer), "┃%*d", numcolw, r + 1);
		if (!bufferWrite(line, numcolw + boxw, buffer)) goto cleanup;
		for (int c = 0; c < cols; c++) {
			char * s = strtab[r * cols + c];
			if (!s) {
				snprintf(buffer, sizeof(buffer), "┃%*s", colw[c], "");
			} else {
				snprintf(buffer, sizeof(buffer), "┃%*s", colw[c], strtab[r * cols + c]);
			}
			if (!bufferWrite(line, colw[c] + boxw, buffer)) goto cleanup;
		}
		if (!bufferWrite(line, boxw, "┃")) goto cleanup;
		puts(line->data);
	}

	// post data
	bufferReset(line);
	for (int c = -1; c < cols; c++) {
		int len;
		if (c == -1) {
			if (!bufferWrite(line, boxw, "┗")) goto cleanup;
			len = numcolw;
		} else {
			if (!bufferWrite(line, boxw, "┻")) goto cleanup;
			len = colw[c];
		}
		for (int i = 0; i < len; i++) {
			if (!bufferWrite(line, boxw, "━")) goto cleanup;
		}
	}
	if (!bufferWrite(line, boxw, "┛")) goto cleanup;
	if (!bufferWrite(line, 1, "\n")) goto cleanup;
	fputs(line->data, fp);

cleanup:
	bufferFree(line);
}
