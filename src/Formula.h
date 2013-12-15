#ifndef FORMULA_H
#define FORMULA_H

#include "Object.h"
#include "Table.h"
#include "Buffer.h"

/*
 * formula := '=' value
 * value := CELLID | NUMBER | STRING | range | call | pattern
 * call := ID '(' value_list ')'
 * value_list := value | value_list ',' value
 * range := reference ':' reference
 * direction := INTEGER
 * pattern := '[' direction , direction ']'
 *
 * NUMBER = -?[0-9.]... and parses as double :-)
 * CELLID = [A-Z][1-9][0-9]*
 * ID = [a-z0-9_]+
 */

typedef enum {
	TOK_NONE,
	TOK_COMMA,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_EQUALS,
	TOK_COLON,
	TOK_SEMICOLON,
	TOK_ID,
	TOK_CELLID,
	TOK_NUMBER,
	TOK_STRING,
	TOK_LBRACKET,
	TOK_RBRACKET,
} TokenType;

typedef struct {
	struct Token {
		TokenType type;
		int len;
		char * value;
		double number;
	} c;

	char * bs; // start;
	char * bp; // position;
	char * be; // end;
} Tokenizer;

void tokenizerInit(Tokenizer * tt, Buffer * value);
APIUSE TokenType tokenizerNext(Tokenizer * tt);
void tokenizerSet(Tokenizer *tt, char * p);

APIUSE Object * newFormula(Collector * c, Buffer * value);

#endif /* FORMULA_H */
