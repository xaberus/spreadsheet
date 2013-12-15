#include "Formula.h"
#include "List.h"

#include "Double.h"
#include "String.h"
#include "Coord.h"
#include "Call.h"
#include "Range.h"
#include "Pattern.h"


#include <string.h>
#include <math.h>

const char * tokenName(TokenType t) {
	switch (t) {
		case TOK_NONE:
			return "<none>";
		case TOK_COMMA:
			return "<,>";
		case TOK_LPAREN:
			return "<(>";
		case TOK_RPAREN:
			return "<)>";
		case TOK_LBRACKET:
			return "<[>";
		case TOK_RBRACKET:
			return "<]>";
		case TOK_EQUALS:
			return "<=>";
		case TOK_COLON:
			return "<:>";
		case TOK_SEMICOLON:
			return "<;>";
		case TOK_ID:
			return "<id>";
		case TOK_CELLID:
			return "<cellid>";
		case TOK_NUMBER:
			return "<number>";
		case TOK_STRING:
			return "<string>";
	}
	return NULL;
}

void tokenizerInit(Tokenizer * tt, Buffer * value) {
	tt->bp = tt->bs = value->data;
	tt->be = tt->bs + value->usage;

	tt->c.type = TOK_NONE;
	tt->c.len = -1;
	tt->c.value = NULL;
}

TokenType tokenizerInvalid(Tokenizer * tt) {
	tt->c.type = TOK_NONE;
	tt->c.len = -1;
	tt->c.value = NULL;
	return tt->c.type;
}

TokenType tokenizerNumber(Tokenizer * tt) {

	int sign = 0;
	double ipart = 0;
	double fpart = 0;
	double fmult = 1;
	int esign = 0;
	double exp = 0;

	enum {
		STATE_START,
		STATE_MINUS,
		STATE_HEAD,
		STATE_TAIL,
		STATE_ZERO,
		STATE_DOT,
		STATE_EXP,
		STATE_ENEG,
		STATE_ETAIL,
	} state = STATE_START;

	tt->c.type = TOK_NUMBER;
	tt->c.len = 0;
	tt->c.value = tt->bp;

	while (tt->bp < tt->be) {
		char c = *tt->bp;

		switch (state) {
			case STATE_START: {
				switch (c) {
					case '0':
						state = STATE_ZERO;
						goto next;
					case '-':
						sign = 1;
						state = STATE_MINUS;
						goto next;
					case '.':
						state = STATE_DOT;
						goto next;
					case '1'...'9':
						ipart = (c - '0');
						state = STATE_HEAD;
						goto next;
					default:
						goto out;
				}
			} break;
			case STATE_MINUS: {
				switch (c) {
					case '0':
						state = STATE_ZERO;
						goto next;
					case '.':
						state = STATE_DOT;
						goto next;
					case '1'...'9':
						ipart = (c - '0');
						state = STATE_HEAD;
						goto next;
					default:
						goto out;
				}
			} break;
			case STATE_HEAD: {
				switch (c) {
					case '.':
						state = STATE_DOT;
						goto next;
					case 'e':
					case 'E':
						state = STATE_EXP;
						goto next;
					case '0'...'9':
						ipart = ipart * 10 + (c - '0');
						state = STATE_HEAD;
						goto next;
					default:
						goto out;
				}
			} break;
			case STATE_TAIL: {
				switch (c) {
					case 'e':
					case 'E':
						state = STATE_EXP;
						goto next;
					case '0'...'9':
						fpart = fpart * 10 + (c - '0');
						fmult = fmult * 10;
						state = STATE_TAIL;
						goto next;
					default:
						goto out;
				}
			} break;
			case STATE_ZERO: {
				switch (c) {
					case '.':
						state = STATE_DOT;
						goto next;
					default:
						goto out;
				}
			} break;
			case STATE_DOT: {
				switch (c) {
					case '0'...'9':
						fpart = fpart * 10 + (c - '0');
						fmult = fmult * 10;
						state = STATE_TAIL;
						goto next;
					default:
						goto out;
				}
			} break;
			case STATE_EXP: {
				switch (c) {
					case '-':
						esign = 1;
						state = STATE_ENEG;
						goto next;
					case '1'...'9':
						exp = (c - '0');
						state = STATE_ETAIL;
						goto next;
					default:
						goto out;
				}
			} break;
			case STATE_ENEG: {
				switch (c) {
					case '1'...'9':
						exp = (c - '0');
						state = STATE_ETAIL;
						goto next;
					default:
						goto out;
				}
			} break;
			case STATE_ETAIL: {
				switch (c) {
					case '0'...'9':
						exp = exp * 10 + (c - '0');
						state = STATE_ETAIL;
						goto next;
					default:
						goto out;
				}
			} break;
		}
next:
		tt->bp++;
		tt->c.len++;
	}
out:
	switch (state) {
		case STATE_HEAD:
		case STATE_TAIL:
		case STATE_ZERO:
			// accept;
			tt->c.number = (sign ? -1. : 1.) * (ipart + fpart/fmult);
			return tt->c.type;
		case STATE_ETAIL:
			// accept;
			tt->c.number =
				(sign ? -1. : 1.) * ((ipart + fpart/fmult)
					* pow(10, (esign ? -1. : 1.) * exp));
			return tt->c.type;
		default:
			// reject;
			return tokenizerInvalid(tt);
	}
	/* not reachable */
}

TokenType tokenizerString(Tokenizer * tt) {
	tt->bp++;
	tt->c.type = TOK_STRING;
	tt->c.len = 0;
	tt->c.value = tt->bp;
	while (tt->bp < tt->be) {
		char c = *tt->bp++;
		if (c == '"') goto accept;
		tt->c.len++;
	}
	return tokenizerInvalid(tt);
	accept:
	return tt->c.type;
}

TokenType tokenizerId(Tokenizer * tt) {
	tt->c.type = TOK_ID;
	tt->c.len = 1;
	tt->c.value = tt->bp;
	tt->bp++;

	while (tt->bp < tt->be) {
		char c = *tt->bp;
		if (('0' <= c && c <= '9') || c == '_' || ('a' <= c && c <= 'z')) {
			tt->bp++;
			tt->c.len++;
			continue;
		}
		break;
	}

	return tt->c.type;
}

TokenType tokenizerCellId(Tokenizer * tt) {
	enum {
		STATE_START,
		STATE_LETTER,
		STATE_TAIL,
	} state = STATE_START;

	double n = 0;

	tt->c.type = TOK_CELLID;
	tt->c.len = 0;
	tt->c.value = tt->bp;

	while (tt->bp < tt->be) {
		char c = *tt->bp;
		switch (state) {
			case STATE_START: {
				switch (c) {
					case 'A'...'Z':
						state = STATE_LETTER;
						goto next;
					default:
						goto out;
				}
			} break;
			case STATE_LETTER: {
				switch (c) {
					case '1'...'9':
						n = n * 10 + (c - '0');
						state = STATE_TAIL;
						goto next;
					default:
						goto out;
				}
			} break;
			case STATE_TAIL: {
				switch (c) {
					case '0'...'9':
						n = n * 10 + (c - '0');
						state = STATE_TAIL;
						goto next;
					default:
						goto out;
				}
			} break;
		}
next:
		tt->bp++;
		tt->c.len++;
	}
out:
	if (state != STATE_TAIL) return tokenizerInvalid(tt);
	tt->c.number = n;
	return tt->c.type;
}

TokenType tokenizerNext(Tokenizer * tt) {
	if (tt->bp >= tt->be) return TOK_NONE;
	char c;
again:
	c = *tt->bp;
	switch (c) {
		case ' ':
		case '\t':
			tt->bp++;
			goto again;
		case '=':
			tt->bp++;
			tt->c.type = TOK_EQUALS;
			break;
		case '(':
			tt->bp++;
			tt->c.type = TOK_LPAREN;
			break;
		case ')':
			tt->bp++;
			tt->c.type = TOK_RPAREN;
			break;
		case '[':
			tt->bp++;
			tt->c.type = TOK_LBRACKET;
			break;
		case ']':
			tt->bp++;
			tt->c.type = TOK_RBRACKET;
			break;
		case ',':
			tt->bp++;
			tt->c.type = TOK_COMMA;
			break;
		case ':':
			tt->bp++;
			tt->c.type = TOK_COLON;
			break;
		case ';':
			tt->bp++;
			tt->c.type = TOK_SEMICOLON;
			break;
		case 'A'...'Z':
			return tokenizerCellId(tt);
		case 'a'...'z':
			return tokenizerId(tt);
		case '0'...'9':
		case '.':
		case '-':
			return tokenizerNumber(tt);
		case '"':
			return tokenizerString(tt);
		default:
			return tokenizerInvalid(tt);
	}

	tt->c.len = -1;
	tt->c.value = NULL;
	return tt->c.type;
}

void tokenizerSet(Tokenizer *tt, char * p) {
	tt->bp = p;
	tt->c.type = TOK_NONE;
	tt->c.len = -1;
	tt->c.value = NULL;
}

#include <stdio.h>

Coord * parseReference(Collector * c, Tokenizer * tt) {
	if (tt->c.len < 2) {
		printf("invalid cell specification\n");
		return NULL;
	}

	int col = tt->c.value[0] - 'A';
	int row = tt->c.number - 1;

	if (col >= 0 && col < 26 && row >= 0) return newCoord(c, col, row);
	return NULL;
}

Object * parseRange(Collector * c, Tokenizer * tt, Coord * ref1) {
	Coord * ref2 = NULL;

	TokenType t = tokenizerNext(tt); if (t != TOK_CELLID) goto fail;
	ref2 = parseReference(c, tt); if (!ref2) goto fail;
	return (Object *) newRange(c, ref1, ref2);

fail:
	return NULL;
}

Object * parseReferenceOrRange(Collector * c, Tokenizer * tt) {
	Coord * ref = NULL;

	ref = parseReference(c, tt); if (!ref) goto fail;
	char * sp = tt->bp;
	TokenType t = tokenizerNext(tt);
	if (t == TOK_COLON) {
		return parseRange(c, tt, ref);
	} else {
		tokenizerSet(tt, sp);
	}
	return (Object *) ref;

fail:
	return NULL;
}

Object * parseNumber(Collector * c, Tokenizer * tt) {
	return (Object *) newDouble(c, tt->c.number);
}

Object * parseString(Collector * c, Tokenizer * tt) {
	return (Object *) newStringN(c, tt->c.len, tt->c.value);
}

Object * parseValue(Collector * c, Tokenizer * tt);

List * parseValueList(Collector * c, Tokenizer * tt) {
	List * args = newList(c); if (!args) goto fail;

	TokenType t = tokenizerNext(tt);
	if (t != TOK_LPAREN) goto fail;

	char * sp = tt->bp;
	t = tokenizerNext(tt);
	if (t == TOK_RPAREN) return args;

	tokenizerSet(tt, sp);
	do {
		Object * arg = parseValue(c, tt); if (!arg) goto fail;
		if (!listAppend(args, arg)) goto fail;
		t = tokenizerNext(tt);

		if (t == TOK_RPAREN) {
			return args;
		}
	} while (t == TOK_COMMA);

fail:
	return NULL;
}

Object * parseCall(Collector * c, Tokenizer * tt) {
	String * func = NULL;
	List * args = NULL;
	Object * call;

	func = newStringN(c, tt->c.len, tt->c.value);	if (!func) goto fail;
	args = parseValueList(c, tt); if (!args) goto fail;

	call = (Object *) newCall(c, func, args);
	if (!call) goto fail;
	return call;

fail:
	return NULL;
}

inline static int checkInteger(double d) {
	if (floor(d) == d) return 1;
	return 0;
}

// todo implement substitution
Object * parsePattern(Collector * c, Tokenizer * tt) {
	if (tokenizerNext(tt) != TOK_NUMBER) goto fail;
	double dcol = tt->c.number;
	if (tokenizerNext(tt) != TOK_COMMA) goto fail;
	if (tokenizerNext(tt) != TOK_NUMBER) goto fail;
	double drow = tt->c.number;
	if (tokenizerNext(tt) != TOK_RBRACKET) goto fail;

	if (!checkInteger(dcol)) goto fail;
	if (!checkInteger(drow)) goto fail;
	Object * pattern = (Object *) newPattern(c, (int) dcol, (int) drow);
	if (!pattern) goto fail;
	return pattern;

fail:
	return NULL;
}

Object * parseValue(Collector * c, Tokenizer * tt) {
	TokenType t = tokenizerNext(tt);

	switch (t) {
		case TOK_CELLID:
			return parseReferenceOrRange(c, tt);
		case TOK_NUMBER:
			return parseNumber(c, tt);
		case TOK_STRING:
			return parseString(c, tt);
		case TOK_ID:
			return parseCall(c, tt);
		case TOK_LBRACKET:
			return parsePattern(c, tt);
		default:
			return NULL;
	}
}

Object * parseFormula(Collector * c, Tokenizer * tt) {
	TokenType t = tokenizerNext(tt);

	switch (t) {
		case TOK_NUMBER:
			return parseNumber(c, tt);
		case TOK_STRING:
			return parseString(c, tt);
		case TOK_EQUALS:
			return parseValue(c, tt);
		default:
			return (Object *) newStringN(c, tt->be - tt->bs, tt->bs);
	}
}

Object * newFormula(Collector * c, Buffer * value) {
	Tokenizer tt;

	tokenizerInit(&tt, value);

	Object * result = parseFormula(c, &tt);

	if (tt.bp != tt.be) return NULL;
	return result;
}

