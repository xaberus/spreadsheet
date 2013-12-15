#include "Buffer.h"
#include "Table.h"
#include "String.h"
#include "Double.h"
#include "Formula.h"

#include "Strtab.h"

#include "DebugPrintf.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <libgen.h>

#include "linenoise/linenoise.h"

typedef int (*Dispatcher)(Buffer * buffer, Table * table);

static int dispatchPrint(Buffer * input, Table * table) {
	(void) input;
	int cols = table->cols;
	int rows = table->rows;
	char *strtab[cols * rows];

	for (unsigned i = 0; i < sizeof(strtab)/sizeof(strtab[0]); i++) strtab[i] = NULL;

	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			Cell * cell = tableGet(table, c, r);
			if (!cell) goto cleanup;
			if (cell->value) {
				char * s = objectToString(cell->value);
				if (!s) goto cleanup;
				strtab[r * cols + c] = s;
			}
		}
	}

	strtabPrintUtf8(stdout, cols, rows, strtab);

cleanup:
	for (unsigned i = 0; i < sizeof(strtab)/sizeof(strtab[0]); i++) {
		if (strtab[i]) free(strtab[i]);
	}

	return 1;
}

static int dispatchFormula(Buffer * input, Table * table) {
	(void) input;
	int cols = table->cols;
	int rows = table->rows;
	char *strtab[cols * rows];

	for (unsigned i = 0; i < sizeof(strtab)/sizeof(strtab[0]); i++) strtab[i] = NULL;

	Buffer * buffer = newBuffer(1024);
	if (!buffer) return 1;

	char * s = NULL, * v = NULL;

	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			if (s) free(s); s = NULL;
			if (v) free(v); v = NULL;
			Cell * cell = tableGet(table, c, r);
			bufferReset(buffer);
			if (!cell) goto cleanup;
			if (cell->formula) {
				s = objectToString(cell->formula);
				if (!s) goto cleanup;
				v = objectToString(cell->value);
				if (!v) goto cleanup;
				if (!bufferWrite(buffer, strlen(v), v)) goto cleanup;
				if (!bufferWrite(buffer, 3, " = ")) goto cleanup;
				if (!bufferWrite(buffer, strlen(s), s)) goto cleanup;
				strtab[r * cols + c] = strdup(buffer->data);
			} else if (cell->value) {
				s = objectToString(cell->value);
				if (!s) goto cleanup;
				strtab[r * cols + c] = s;
				s = NULL;
			}
		}
	}

	strtabPrintUtf8(stdout, cols, rows, strtab);

cleanup:
	if (s) free(s);
	if (v) free(v);
	if (buffer) bufferFree(buffer);
	for (unsigned i = 0; i < sizeof(strtab)/sizeof(strtab[0]); i++) {
		if (strtab[i]) free(strtab[i]);
	}

	return 1;
}

static int dispatchSetJoined(Buffer * input, Table * table) {
	Buffer * cmd = NULL, * joined = NULL;

	if (!bufferSplitN(input, ' ', BUFFER_SPLIT_MERGE, 2, &cmd, &joined)) {
		printf("could not parse cell command\n");
		return 1;
	}


	Tokenizer ts;
	tokenizerInit(&ts, joined);

	if(tokenizerNext(&ts) != TOK_CELLID)  {
		printf("could not parse cell command\n");
		goto cleanup;
	}

	int col = ts.c.value[0] - 'A';
	int row = ts.c.number - 1;

	if(tokenizerNext(&ts) != TOK_EQUALS)  {
		printf("could not parse cell command\n");
		goto cleanup;
	}

	Cell * cell = tableGet(table, col, row);
	if (!cell) {
		printf("invalid cell specified\n");
		goto cleanup;
	}

	bufferReset(input);
	if (!bufferWrite(input, ts.be - (ts.bp - 1), ts.bp - 1)) goto cleanup;

	if (!tableSetCell(table, cell, input)) {
		printf("could not set cell value\n");
	}


cleanup:
	bufferFree(cmd);
	bufferFree(joined);

	return 1;
}


static int dispatchSet(Buffer * input, Table * table) {
	Buffer * cmd = NULL, * cellid = NULL, * value = NULL;

	unsigned flags = BUFFER_SPLIT_MERGE | BUFFER_SPLIT_TAKE;
	if (!bufferSplitN(input, ' ', flags, 3, &cmd, &cellid, &value)) {
		return dispatchSetJoined(input, table);
	}


	Tokenizer ts;
	tokenizerInit(&ts, cellid);
	if(tokenizerNext(&ts) != TOK_CELLID) {
		printf("invalid cell specification\n");
		goto cleanup;
	}

	int col = ts.c.value[0] - 'A';
	int row = ts.c.number - 1;

	Cell * cell = tableGet(table, col, row);
	if (!cell) {
		printf("invalid cell specified\n");
		goto cleanup;
	}

	if (!tableSetCell(table, cell, value)) {
		printf("could not set cell value\n");
	}

cleanup:
	bufferFree(cmd);
	bufferFree(cellid);
	bufferFree(value);

	return 1;
}

static int dispatchEval(Buffer * input, Table * table) {
	Buffer * cmd = NULL, * value = NULL;

	unsigned flags = BUFFER_SPLIT_MERGE | BUFFER_SPLIT_TAKE;
	if (!bufferSplitN(input, ' ', flags, 2, &cmd, &value)) return 1;

	Object * fml = newFormula(table->c, value);
	if (fml) {
		Object * res = tableEvaluateFormula(table, NULL, fml);
		if (res) {
			debugPrintf(stdout, "= %o\n", res);
		}
	} else {
		printf("could not parse formula '%.*s'\n", (int) value->usage, value->data);
	}

	bufferFree(cmd);
	bufferFree(value);

	return 1;
}

static int dispatchDepProp(Buffer * input, Table * table) {
	(void) input;
	int cols = table->cols;
	int rows = table->rows;
	char *strtab[cols * rows];

	Buffer * buff = newBuffer(1024);
	if (!buff) return 1;

	for (unsigned i = 0; i < sizeof(strtab)/sizeof(strtab[0]); i++) strtab[i] = NULL;

	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			bufferReset(buff);
			Cell * cell = tableGet(table, c, r);

			if (listIsEmpty(cell->depends) && listIsEmpty(cell->propagate)) continue;

			if (!cell) goto cleanup;
			char * d = objectToString((Object *) cell->depends);
			if (!d) goto cleanup;
			if (!bufferWrite(buff, strlen(d), d)) {
				free(d);
				goto cleanup;
			}
			free(d);

			if (!bufferWrite(buff, 2, "->")) goto cleanup;

			char * p = objectToString((Object *) cell->propagate);
			if (!p) goto cleanup;
			if (!bufferWrite(buff, strlen(p), p)) {
				free(p);
				goto cleanup;
			}
			free(p);

			char * s = strndup(buff->data, buff->usage);
			if (!s) goto cleanup;

			strtab[r * cols + c] = s;
		}
	}

	strtabPrintUtf8(stdout, cols, rows, strtab);

cleanup:
	for (unsigned i = 0; i < sizeof(strtab)/sizeof(strtab[0]); i++) {
		if (strtab[i]) free(strtab[i]);
	}
	if (buff) bufferFree(buff);

	return 1;
}

static int dispatchExport(Buffer * input, Table * table) {
	Buffer * cmd = NULL, * value = NULL;

	if (!bufferSplitN(input, ' ', BUFFER_SPLIT_MERGE, 2, &cmd, &value)) {
		printf("could not parse export command\n");
		return 1;
	}

	if (!tableExport(table, value->data)) {
		printf("could not export table\n");
	}

	if (cmd) bufferFree(cmd);
	if (value) bufferFree(value);

	return 1;
}

static int dispatchImport(Buffer * input, Table * table) {
	Buffer * cmd = NULL, * value = NULL;

	if (!bufferSplitN(input, ' ', BUFFER_SPLIT_MERGE, 2, &cmd, &value)) {
		printf("could not parse import command\n");
		return 1;
	}

	if (!tableImport(table, value->data)) {
		printf("could not import table\n");
	}

	if (cmd) bufferFree(cmd);
	if (value) bufferFree(value);

	return 1;
}

int parseLine(Buffer * buffer, Table * table);

static int dispatchRead(Buffer * input, Table * table) {
	Buffer * cmd = NULL, * value = NULL;

	if (!bufferSplitN(input, ' ', BUFFER_SPLIT_MERGE, 2, &cmd, &value)) {
		printf("could not parse read command\n");
		return 1;
	}

	FILE * fp = fopen(value->data, "rb");
	if (!fp) {
		printf("could not read file '%s'\n", value->data);
		goto cleanup;
	}

	while (!feof(fp)) {
		bufferReset(input);
		if (!bufferReadLine(input, fp)) break;
		if (bufferIsEmpty(input)) continue;
		if (input->data[0] == '#') continue;
		if (!parseLine(input, table)) break;
		collectorRun(table->c);
	}
	fclose(fp);

cleanup:
	if (cmd) bufferFree(cmd);
	if (value) bufferFree(value);

	return 1;
}

static int dispatchWrite(Buffer * input, Table * table) {
	(void) table;
	Buffer * cmd = NULL, * value = NULL;

	if (!bufferSplitN(input, ' ', BUFFER_SPLIT_MERGE, 2, &cmd, &value)) {
		printf("could not parse read command\n");
		return 1;
	}

	if (!linenoiseHistorySave(value->data) != 0) {
		printf("could not write file '%s'\n", value->data);
	}

	if (cmd) bufferFree(cmd);
	if (value) bufferFree(value);

	return 1;
}

static int dispatchClear(Buffer * input, Table * table) {
	Buffer * cmd = NULL, * width = NULL, * height = NULL;

	if (!bufferSplitN(input, ' ', BUFFER_SPLIT_MERGE, 3, &cmd, &width, &height)) {
		printf("could not parse clear command\n");
		return 1;
	}

	int cols = atoi(width->data);
	int rows = atoi(height->data);

	if (cols < 0 && rows < 0) {
		printf("invalid table size!\n");
		goto cleanup;
	}

	tableClear(table);
	if (!tableResize(table, cols, rows)) {
		printf("could not resize table!\n");
		goto cleanup;
	}

cleanup:
	if (cmd) bufferFree(cmd);
	if (width) bufferFree(width);
	if (height) bufferFree(height);
	return 1;
}



static int dispatchExit(Buffer * buffer, Table * table) {
	(void) table;
	(void) buffer;
	printf("exiting\n");
	return 0;
}

typedef void (*Completer)(size_t clen, const char * buf, linenoiseCompletions *lc);

void completeFileName(size_t clen, const char * buf, linenoiseCompletions *lc) {

	const char * path = buf + clen + 1;

	DIR * dd = NULL;
	char * pdir = NULL;
	char * pname = NULL;

	pdir = strdup(path);
	if (!pdir) goto cleanup;
	pname = strdup(path);
	if (!pname) goto cleanup;

	char * dir = dirname(pdir);
	char * name = basename(pname);

	if (strcmp(name, ".") == 0) {
		free(pname);
		name = pname = strdup("");
		if (!pname) goto cleanup;
	}

	//printf("path %s -> '%s' and '%s'\n", path, dir, name);

	struct dirent * de;
	dd = opendir(dir);
	if (!dd) goto cleanup;

	size_t nlen = strlen(name);
	size_t dlen = strlen(dir);

	while ((de = readdir(dd)) != NULL) {
		char entry[clen + 1 + dlen + 1 + strlen(de->d_name) + 1 + 1];
		snprintf(entry, sizeof(entry), "%s/%s", dir, de->d_name);
		struct stat stbuf;
		if (stat(entry, &stbuf) == -1) continue;


		if (strncmp(de->d_name, name, nlen) == 0) {
			if ((stbuf.st_mode & S_IFMT ) == S_IFDIR) {
				snprintf(entry, sizeof(entry), "%.*s %s/%s/", (int) clen, buf, dir, de->d_name);
			} else {
				snprintf(entry, sizeof(entry), "%.*s %s/%s", (int) clen, buf, dir, de->d_name);
			}
			linenoiseAddCompletion(lc, entry);
		}
	}

cleanup:
	if (dd) closedir(dd);
	if (pdir) free(pdir);
	if (pname) free(pname);
}

static struct {
	const char * cmd;
	Dispatcher fn;
	Completer cn;
	int exact;
	int breve;
} callTable[] = {
	{"exit", dispatchExit, NULL, 1, 0},
	{"q", dispatchExit, NULL, 1, 1},
	{"print", dispatchPrint, NULL, 1, 0},
	{"p", dispatchPrint, NULL, 1, 1},
	{"formula", dispatchFormula, NULL, 1, 0},
	{"f", dispatchFormula, NULL, 1, 1},
	{"set", dispatchSet, NULL, 0, 0},
	{"s", dispatchSet, NULL, 0, 1},
	{"eval", dispatchEval, NULL, 0, 0},
	{"e", dispatchEval, NULL, 0, 1},
	{"depprop", dispatchDepProp, NULL, 1, 0},
	{"dp", dispatchDepProp, NULL, 1, 1},
	{"export", dispatchExport, completeFileName, 0, 0},
	{"import", dispatchImport, completeFileName, 0, 0},
	{"read", dispatchRead, completeFileName, 0, 0},
	{"write", dispatchWrite, completeFileName, 0, 0},
	{"clear", dispatchClear, NULL, 0, 0},
};

void dispatchNone(Buffer * buffer) {
	printf("unknown command '%.*s'\n", (int) buffer->usage, buffer->data);
}

int parseLine(Buffer * buffer, Table * table) {
	//printf("# input was '%.*s'\n", (int) buffer->usage, buffer->data);

	for (unsigned i = 0; i < sizeof(callTable)/sizeof(callTable[0]); i++) {
		if (callTable[i].exact) {
			if (strlen(callTable[i].cmd) != buffer->usage) continue;
			if (strcmp(callTable[i].cmd, buffer->data) != 0) continue;
			return callTable[i].fn(buffer, table);
		} else {
			size_t len = strlen(callTable[i].cmd);
			if (buffer->usage < len) continue;
			if (strncmp(callTable[i].cmd, buffer->data, len) != 0) continue;
			if (buffer->usage == len) {
				printf("command '%s' needs arguments\n", callTable[i].cmd);
				return 1;
			}
			if (buffer->data[len] != ' ') continue;
			return callTable[i].fn(buffer, table);
		}
	}

	dispatchNone(buffer);

	return 1;
}

inline static int min(int x, int y) {
	return x > y ? y : x;
}

void completion(const char * buf, linenoiseCompletions * lc) {
	for (unsigned i = 0; i < sizeof(callTable)/sizeof(callTable[0]); i++) {
		if (callTable[i].breve) continue;
		size_t blen = strlen(buf);
		size_t clen = strlen(callTable[i].cmd);
		if (strncmp(buf, callTable[i].cmd, min(blen, clen)) == 0) {
			if (blen > clen && buf[clen] == ' ') {
				if (callTable[i].cn) {
					callTable[i].cn(clen, buf, lc);
				}
				break;
			} else if (blen == clen) {
				char tmp[blen + 2];
				snprintf(tmp, sizeof(tmp), "%s ", callTable[i].cmd);
				linenoiseAddCompletion(lc, tmp);
			} else {
				linenoiseAddCompletion(lc, callTable[i].cmd);
			}
		}
	}
}

int main(int argc, char * argv[]) {
	Buffer * buffer = NULL;
	Collector * col = NULL;
	Table * table = NULL;
	const char * file = NULL;

	buffer = newBuffer(1024);
	if (!buffer) {
		printf("could not allocate read buffer\n");
		goto cleanup;
	}
	col = newCollector();
	if (!col) {
		printf("could not allocate garbage collector\n");
		goto cleanup;
	}
	table = newTable(col, 0, 0);
	if (!table) {
		printf("could not allocate table\n");
		goto cleanup;
	}

	if (!collectorRoot(col, (Object *) table)) {
		printf("could not set table as collector root\n");
	}

	if (argc > 1) {
		printf("inporting table from '%s'\n", argv[1]);
		if (!tableImport(table, argv[1])) {
			printf("could not import table\n");
			tableClear(table);
			if (!tableResize(table, 10, 10)) {
				printf("could not resize table!\n");
				goto cleanup;
			}
		}
		file = argv[1];
		dispatchPrint(NULL, table);
	} else {
		if (!tableResize(table, 10, 10)) {
			printf("could not resize table!\n");
			goto cleanup;
		}
	}

	if (isatty(STDIN_FILENO)) {
		linenoiseSetCompletionCallback(completion);
		char * line = NULL;
		int done = 0;
		while (!done && (line = linenoise("> "))) {
			bufferReset(buffer);
			if (!bufferWrite(buffer, strlen(line), line)) goto next;
			if (bufferIsEmpty(buffer)) goto next;
			if (buffer->data[0] == '#') goto next;
			if (!parseLine(buffer, table)) {
				done = 1;
				goto next;
			}
			linenoiseHistoryAdd(line);
next:
			if (file) {
				// really?
				if (!tableExport(table, file)) {
					printf("could not export table\n");
				}
			}
			collectorRun(col);
			free(line);
		}
	} else {
		while (!feof(stdin)) {
			bufferReset(buffer);
			if (!bufferReadLine(buffer, stdin)) break;
			if (bufferIsEmpty(buffer)) continue;
			if (buffer->data[0] == '#') continue;
			if (!parseLine(buffer, table)) break;
			collectorRun(col);
		}
	}

cleanup:
	bufferFree(buffer);
	collectorFree(col);
	return 0;
}
